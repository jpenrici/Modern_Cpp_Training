#!/usr/bin/env perl
# manage.pl - project helper: setup | check | build | test | run | demo | trace | clean
# Uses Perl core modules only.
#
# 'trace' walks the whole pipeline step by step (manage.pl -> gen_errors.pl ->
# CMake -> cargo -> ffi_cpp -> executables -> loader -> tests -> demo), explains
# each stage, runs it for real and inspects the artifacts it produced.
use v5.40;

use Cwd        qw(abs_path);
use File::Path qw(remove_tree);
use File::Spec;
use FindBin      qw($RealBin);
use Getopt::Long qw(GetOptionsFromArray);
use List::Util   qw(first max);
use POSIX        ();
use Time::HiRes  qw(time);

my $ROOT      = abs_path( File::Spec->catdir( $RealBin, File::Spec->updir ) );
my $GENERATOR = File::Spec->catfile( $ROOT, 'scripts', 'gen_errors.pl' );

my %opt = (
    build_dir => File::Spec->catdir( $ROOT, 'build' ),
    type      => 'Release',
    jobs      => 0,
    all       => 0,
    fresh     => 0,
    pause     => 0,
    verbose   => 0,
);

my @REQUIRED_FILES = qw(
  CMakeLists.txt
  common/CMakeLists.txt
  common/error_codes.txt
  common/include/ffi_bridge/ffi_api.h
  rust/Cargo.toml
  rust/CMakeLists.txt
  rust/build.rs
  rust/src/lib.rs
  rust/src/ffi.rs
  rust/src/errors.rs
  rust/src/string_ops.rs
  rust/src/callback.rs
  rust/tests/ffi_tests.rs
  cpp/CMakeLists.txt
  cpp/src/main.cpp
  cpp/tests/test_main.cpp
  scripts/gen_errors.pl
);

my @GENERATED_FILES = qw(
  common/include/ffi_bridge/error_codes.h
  rust/src/error_codes.rs
  cpp/include/ffi_bridge/error_table.hpp
);

# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------

sub info($msg) { say "[manage] $msg"; return; }

sub fail($msg) { die "[manage] ERROR: $msg\n"; }

# Runs a command (list form, no shell) and dies on failure.
sub run_cmd(@cmd) {
    say "+ @cmd";
    system { $cmd[0] } @cmd;
    fail("cannot execute '$cmd[0]': $!")                 if $? == -1;
    fail( "'$cmd[0]' killed by signal " . ( $? & 127 ) ) if $? & 127;
    my $code = $? >> 8;
    fail("command failed with exit code $code: @cmd") if $code;
    return;
}

sub find_in_path($name) {
    return ( -x $name ? $name : undef ) if $name =~ m{/};
    for my $dir ( split /:/, $ENV{PATH} // '' ) {
        next if $dir eq '';
        my $candidate = File::Spec->catfile( $dir, $name );
        return $candidate if -f $candidate && -x _;
    }
    return undef;
}

# Captures stdout of a command; returns undef when it cannot be run.
sub capture(@cmd) {
    return undef unless defined find_in_path( $cmd[0] );
    my $out;
    my $ok = eval {
        open( my $fh, '-|', @cmd ) or die "cannot run $cmd[0]: $!\n";
        local $/;
        $out = <$fh>;
        close($fh);
        1;
    };
    return $ok ? $out : undef;
}

# Runs a command with stdout+stderr merged; never dies on a non-zero exit.
# Optional $cwd changes the child's working directory. Returns ($exit_code, $output).
sub run_capture_in( $cwd, @cmd ) {
    pipe( my $reader, my $writer ) or fail("pipe failed: $!");
    my $pid = fork();
    fail("fork failed: $!") unless defined $pid;
    if ( $pid == 0 ) {
        close $reader;
        if ( defined $cwd ) {
            chdir $cwd or POSIX::_exit(126);
        }
        open( STDOUT, '>&', $writer ) or POSIX::_exit(126);
        open( STDERR, '>&', $writer ) or POSIX::_exit(126);
        close $writer;
        exec { $cmd[0] } @cmd or do {
            print STDERR "cannot execute '$cmd[0]': $!\n";
            POSIX::_exit(127);
        };
    }
    close $writer;
    my $output = do { local $/; <$reader> }
      // '';
    close $reader;
    waitpid( $pid, 0 );
    my $code = ( $? & 127 ) ? 128 + ( $? & 127 ) : ( $? >> 8 );
    return ( $code, $output );
}

sub version_ge( $have, $want ) {
    my @h = split /\./, $have;
    my @w = split /\./, $want;
    for my $i ( 0 .. max( scalar(@h), scalar(@w) ) - 1 ) {
        my ( $x, $y ) = ( $h[$i] // 0, $w[$i] // 0 );
        return true  if $x > $y;
        return false if $x < $y;
    }
    return true;
}

sub default_jobs() {
    my $out = capture('nproc');
    return ( $out && $out =~ /^(\d+)/ ) ? $1 : 2;
}

sub run_generator(@args) {
    run_cmd( $^X, $GENERATOR, @args );
    return;
}

sub rel($path) { return File::Spec->abs2rel( $path, $ROOT ); }

sub guard_build_dir() {
    my $build = $opt{build_dir};
    fail("refusing to remove '$build'")
      if $build eq $ROOT || $build eq '/' || $build eq '';
    return $build;
}

# Toolchain / project verification shared by 'check' and 'trace'.
# Returns a list of [ok, label, detail].
sub collect_checks($with_generated) {
    my @results;
    my $add = sub ( $ok, $label, $detail ) {
        push @results, [ ( $ok ? 1 : 0 ), $label, $detail ];
        return;
    };

    my $perl_v = sprintf( '%vd', $^V );
    $add->( version_ge( $perl_v, '5.40.0' ), 'perl >= 5.40', $perl_v );

    my ($cmake_v) =
      ( capture( 'cmake', '--version' ) // '' ) =~ /version\s+(\d+(?:\.\d+)*)/;
    $add->(
        ( defined $cmake_v && version_ge( $cmake_v, '3.25' ) ),
        'cmake >= 3.25',
        $cmake_v // 'not found'
    );

    my ($rustc_v) =
      ( capture( 'rustc', '--version' ) // '' ) =~ /rustc\s+(\d+(?:\.\d+)*)/;
    $add->(
        ( defined $rustc_v && version_ge( $rustc_v, '1.85' ) ),
        'rustc >= 1.85',
        $rustc_v // 'not found'
    );

    my ($cargo_v) =
      ( capture( 'cargo', '--version' ) // '' ) =~ /cargo\s+(\d+(?:\.\d+)*)/;
    $add->( defined $cargo_v, 'cargo', $cargo_v // 'not found' );

    my $cxx = $ENV{CXX}
      // first { defined find_in_path($_) } qw(c++ g++ clang++);
    if ( !defined $cxx ) {
        $add->( false, 'C++23 compiler', 'not found' );
    }
    else {
        my $banner = capture( $cxx, '--version' ) // '';
        if ( $banner =~ /clang version\s+(\d+(?:\.\d+)*)/ ) {
            $add->( version_ge( $1, '17' ), 'clang >= 17', "$cxx $1" );
        }
        elsif ( defined( my $gcc_v = capture( $cxx, '-dumpversion' ) ) ) {
            chomp $gcc_v;
            $add->( version_ge( $gcc_v, '13' ), 'gcc >= 13', "$cxx $gcc_v" );
        }
        else {
            $add->( false, 'C++23 compiler', "cannot query $cxx" );
        }
    }

    my @missing = grep { !-f File::Spec->catfile( $ROOT, $_ ) } @REQUIRED_FILES;
    $add->(
        scalar(@missing) == 0,
        'project files',
        @missing
        ? 'missing: ' . join( ', ', @missing )
        : scalar(@REQUIRED_FILES) . ' present',
    );

    if ($with_generated) {
        my ( $code, $output ) =
          run_capture_in( undef, $^X, $GENERATOR, '--check' );
        $output =~ s/\s+\z//;
        $add->(
            $code == 0,
            'generated files up to date',
            $code == 0 ? '' : "run: manage.pl run ($output)"
        );
    }
    return @results;
}

# ----------------------------------------------------------------------------
# Commands
# ----------------------------------------------------------------------------

sub cmd_check(@args) {
    info('checking toolchain, project files and generated dictionaries');
    my $failures = 0;
    for my $r ( collect_checks(true) ) {
        my ( $ok, $label, $detail ) = @$r;
        printf "  [%-4s] %-34s %s\n", ( $ok ? 'OK' : 'FAIL' ), $label, $detail;
        $failures++ unless $ok;
    }
    if ($failures) {
        info("check finished with $failures problem(s)");
        return 1;
    }
    info('all checks passed');
    return 0;
}

sub cmd_run(@args) {
    run_generator(@args);
    return 0;
}

sub cmd_setup(@args) {
    run_generator();
    run_cmd( 'cmake', '-S', $ROOT, '-B', $opt{build_dir},
        "-DCMAKE_BUILD_TYPE=$opt{type}" );
    return 0;
}

sub cmd_build(@args) {
    cmd_setup()
      unless -f File::Spec->catfile( $opt{build_dir}, 'CMakeCache.txt' );
    my $jobs = $opt{jobs} > 0 ? $opt{jobs} : default_jobs();
    run_cmd( 'cmake', '--build', $opt{build_dir}, '--parallel', $jobs );
    return 0;
}

sub cmd_test(@args) {
    cmd_build();
    run_cmd( 'ctest', '--test-dir', $opt{build_dir}, '--output-on-failure' );
    return 0;
}

sub cmd_demo(@args) {
    cmd_build();
    run_cmd( File::Spec->catfile( $opt{build_dir}, 'cpp', 'ffi_bridge_demo' ) );
    return 0;
}

sub cmd_clean(@args) {
    my $build = guard_build_dir();

    for my $dir ( $build, File::Spec->catdir( $ROOT, 'rust', 'target' ) ) {
        next unless -d $dir;
        info("removing $dir");
        remove_tree($dir);
    }
    if ( $opt{all} ) {
        for my $rel ( @GENERATED_FILES, 'rust/Cargo.lock' ) {
            my $path = File::Spec->catfile( $ROOT, $rel );
            next unless -e $path;
            info("removing $rel");
            unlink $path or fail("cannot remove $path: $!");
        }
    }
    return 0;
}

# ----------------------------------------------------------------------------
# trace: guided walk through the whole pipeline
# ----------------------------------------------------------------------------

my @TRACE;    # [name, status, seconds] per step, for the final summary

sub show_cmd(@cmd) {
    say "  \$ @cmd";
    return;
}

# Prints captured output indented. Unless --verbose (or all => 1) it keeps only
# lines matching `match` and at most `limit` of them.
sub show_output( $text, %o ) {
    my @lines = split /\n/, $text;
    my $extra = 0;
    unless ( $o{all} || $opt{verbose} ) {
        @lines = grep { $_ =~ $o{match} } @lines if defined $o{match};
        my $limit = $o{limit} // 12;
        if ( @lines > $limit ) {
            $extra = @lines - $limit;
            @lines = @lines[ 0 .. $limit - 1 ];
        }
    }
    say "    | $_" for @lines;
    say "    | ... ($extra more line(s); use --verbose to see everything)"
      if $extra;
    return;
}

sub count_lines($path) {
    open( my $fh, '<', $path ) or return 0;
    my $count = 0;
    $count++ while <$fh>;
    close $fh;
    return $count;
}

sub read_error_entries($path) {
    open( my $fh, '<', $path ) or fail("cannot open $path: $!");
    my @entries;
    while ( my $line = <$fh> ) {
        chomp $line;
        next if $line =~ /^\s*(?:#|$)/;
        my ( $code, $name, $message ) = split /\|/, $line, 3;
        next unless defined $message;
        push @entries, [ $code, $name, $message ];
    }
    close $fh;
    return @entries;
}

# Names of the functions declared in ffi_api.h (the C-ABI contract).
sub header_functions() {
    my $path = File::Spec->catfile( $ROOT, 'common', 'include', 'ffi_bridge',
        'ffi_api.h' );
    open( my $fh, '<', $path ) or fail("cannot open $path: $!");
    local $/;
    my $text = <$fh>;
    close $fh;
    return $text =~ /^(?:int32_t|void)\s+(ffi_\w+)\s*\(/mg;
}

sub trace_step( $index, $total, $name, $read, $what, $body ) {
    say '';
    say '=' x 72;
    say "STEP $index/$total  $name";
    say '=' x 72;
    my $label = 'what :';
    for my $line (@$what) {
        say "  $label $line";
        $label = '      ';
    }
    say "  read : " . join( ', ', @$read );

    if ( $opt{pause} && -t STDIN ) {
        print "  press Enter to run this step (q to quit)... ";
        my $answer = <STDIN>;
        if ( !defined $answer || $answer =~ /^\s*q/i ) {
            say "\n[trace] aborted by user";
            return undef;
        }
    }
    say '';

    my $start = time;
    my $ok    = eval { $body->() };
    if ( my $error = $@ ) {
        $error =~ s/\s+\z//;
        say "  ! $error";
        $ok = false;
    }
    my $elapsed = time - $start;
    my $status  = $ok ? 'OK' : 'FAIL';
    printf "\n  result: %s (%.2fs)\n", $status, $elapsed;
    push @TRACE, [ $name, $status, $elapsed ];
    return $ok ? 1 : 0;
}

sub cmd_trace(@args) {
    my $build = guard_build_dir();
    @TRACE = ();

    my $profile = $opt{type} eq 'Debug' ? 'debug'    : 'release';
    my $jobs    = $opt{jobs} > 0        ? $opt{jobs} : default_jobs();
    my $rust_lib =
      File::Spec->catfile( $build, 'rust-target', $profile, 'libffi_rust.so' );
    my $cpp_lib = File::Spec->catfile( $build, 'cpp', 'libffi_cpp.a' );
    my $demo    = File::Spec->catfile( $build, 'cpp', 'ffi_bridge_demo' );
    my $tests   = File::Spec->catfile( $build, 'cpp', 'ffi_cpp_tests' );

    say
'pipeline: manage.pl -> gen_errors.pl -> CMake -> cargo -> ffi_cpp -> executables -> loader -> tests -> demo';
    if ( $opt{fresh} ) {
        say
'[trace] --fresh: removing build dir, rust/target and generated files first';
        for my $dir ( $build, File::Spec->catdir( $ROOT, 'rust', 'target' ) ) {
            remove_tree($dir) if -d $dir;
        }
        for my $rel (@GENERATED_FILES) {
            my $path = File::Spec->catfile( $ROOT, $rel );
            unlink $path if -e $path;
        }
    }
    else {
        say
'[trace] tip: use --fresh to watch every stage do real work (compile, generate, link)';
    }

    my $cmake_build = sub (@targets) {
        my @cmd = (
            'cmake', '--build', $build, '--parallel', $jobs, '--target',
            @targets
        );
        show_cmd(@cmd);
        return run_capture_in( undef, @cmd );
    };

    my @steps = (

        # 1 ------------------------------------------------------------------
        [
            'manage.pl: toolchain',
            ['scripts/manage.pl'],
            [
'manage.pl parses the command and options, then verifies the toolchain',
'(Perl 5.40, CMake 3.25, Rust 1.85, GCC 13 / Clang 17) and the project files.',
            ],
            sub {
                say "  root       : $ROOT";
                say "  build dir  : $build";
                say "  build type : $opt{type}  (cargo profile: $profile)";
                say "  jobs       : $jobs";
                my $bad = 0;
                for my $r ( collect_checks(false) ) {
                    my ( $ok, $label, $detail ) = @$r;
                    printf "    [%-4s] %-24s %s\n", ( $ok ? 'OK' : 'FAIL' ),
                      $label, $detail;
                    $bad++ unless $ok;
                }
                return $bad == 0;
            },
        ],

        # 2 ------------------------------------------------------------------
        [
            'gen_errors.pl: dictionaries',
            [ 'scripts/gen_errors.pl', 'common/error_codes.txt' ],
            [
'One source of truth (error_codes.txt) becomes three artifacts: a C header,',
'a Rust module and a C++ table. Files are rewritten only if content changed.',
            ],
            sub {
                my $source =
                  File::Spec->catfile( $ROOT, 'common', 'error_codes.txt' );
                my @entries = read_error_entries($source);
                say "  source: common/error_codes.txt ("
                  . scalar(@entries)
                  . " entries)";
                printf "    %4s  %-16s %s\n", @$_ for @entries;

                show_cmd( $^X, $GENERATOR, '--check' );
                my ( $c0, $o0 ) =
                  run_capture_in( undef, $^X, $GENERATOR, '--check' );
                if ( $c0 == 0 ) {
                    say '  state before: generated files are up to date';
                }
                else {
                    say '  state before: stale or missing';
                    show_output( $o0, all => 1 );
                }

                show_cmd( $^X, $GENERATOR );
                my ( $c1, $o1 ) = run_capture_in( undef, $^X, $GENERATOR );
                show_output(
                    (
                          $o1 ne ''
                        ? $o1
                        : "(nothing rewritten: content unchanged)\n"
                    ),
                    all => 1
                );
                return false if $c1;

                my $all_present = true;
                for my $rel (@GENERATED_FILES) {
                    my $path = File::Spec->catfile( $ROOT, $rel );
                    if ( -f $path ) {
                        printf "  artifact: %-46s %4d lines, %5d bytes\n",
                          $rel, count_lines($path), -s $path;
                    }
                    else {
                        say "  artifact: $rel MISSING";
                        $all_present = false;
                    }
                }
                my ($c2) = run_capture_in( undef, $^X, $GENERATOR, '--check' );
                say '  verification: '
                  . (
                    $c2 == 0
                    ? 'generated files match the source of truth'
                    : 'STILL STALE'
                  );
                return $all_present && $c2 == 0;
            },
        ],

        # 3 ------------------------------------------------------------------
        [
            'CMake: configure',
            [
                'CMakeLists.txt',      'common/CMakeLists.txt',
                'rust/CMakeLists.txt', 'cpp/CMakeLists.txt'
            ],
            [
'Configure checks that <expected> compiles (C++23), runs gen_errors.pl once more',
'(execute_process) and declares the targets: ffi_common, ffi_rust, ffi_cpp, exes.',
            ],
            sub {
                my @cmd = (
                    'cmake', '-S', $ROOT, '-B', $build,
                    "-DCMAKE_BUILD_TYPE=$opt{type}"
                );
                show_cmd(@cmd);
                my ( $code, $out ) = run_capture_in( undef, @cmd );
                show_output(
                    $out,
                    match =>
qr/compiler identification|FFI_HAVE_STD_EXPECTED|Found Perl|Configuring done|Generating done|Build files|rror/,
                    limit => 12,
                );
                return false if $code;
                my $cache = File::Spec->catfile( $build, 'CMakeCache.txt' );
                say '  artifact: '
                  . rel($cache)
                  . ( -f $cache ? '' : ' MISSING' );
                return -f $cache ? true : false;
            },
        ],

        # 4 ------------------------------------------------------------------
        [
            'cargo: Rust cdylib',
            [ 'rust/CMakeLists.txt', 'rust/build.rs', 'rust/src/ffi.rs' ],
            [
'A custom command runs cargo. build.rs stamps SONAME=libffi_rust.so so the',
'loader can find the library by name. We then check the SONAME and that every',
                'function declared in ffi_api.h is really exported.',
            ],
            sub {
                my ( $code, $out ) = $cmake_build->('ffi_rust_build');
                show_output(
                    $out,
                    match => qr/Building Rust|Compiling|Finished|rror|arning/,
                    limit => 10
                );
                return false if $code;
                return false unless -f $rust_lib;
                printf "  artifact: %s (%d bytes)\n", rel($rust_lib),
                  -s $rust_lib;

                my $ok = true;
                if ( defined find_in_path('readelf') ) {
                    my $dyn = capture( 'readelf', '-d', $rust_lib ) // '';
                    if ( $dyn =~ /\(SONAME\)\s+Library soname:\s+\[([^\]]+)\]/ )
                    {
                        say "  SONAME  : $1";
                        $ok = false unless $1 eq 'libffi_rust.so';
                    }
                    else {
                        say
'  SONAME  : MISSING (the executable would record a relative path)';
                        $ok = false;
                    }
                }
                else {
                    say '  (readelf not found: SONAME check skipped)';
                }

                if ( defined find_in_path('nm') ) {
                    my $symbols =
                      capture( 'nm', '-D', '--defined-only', $rust_lib ) // '';
                    my %exported = map { $_ => 1 } $symbols =~ /\b(ffi_\w+)\b/g;
                    my @wanted   = header_functions();
                    my @missing  = grep { !$exported{$_} } @wanted;
                    say '  exports : '
                      . join( ', ', grep { $exported{$_} } @wanted );
                    if (@missing) {
                        say '  MISSING exports: ' . join( ', ', @missing );
                        $ok = false;
                    }
                }
                else {
                    say '  (nm not found: export check skipped)';
                }
                return $ok;
            },
        ],

        # 5 ------------------------------------------------------------------
        [
            'ffi_cpp: C++ wrappers',
            [
                'cpp/include/ffi_bridge/rust_string.hpp',
                'cpp/src/borrowing.cpp',
                'cpp/src/logging.cpp'
            ],
            [
'Compiles the idiomatic C++23 layer (RustString, borrowing, LogSubscription,',
'ErrorDict) into a static library that links the shared Rust library.',
            ],
            sub {
                my ( $code, $out ) = $cmake_build->('ffi_cpp');
                show_output(
                    $out,
                    match => qr/Building CXX|Linking|Built target|rror|arning/,
                    limit => 10
                );
                return false if $code;
                return false unless -f $cpp_lib;
                printf "  artifact: %s (%d bytes)\n", rel($cpp_lib),
                  -s $cpp_lib;
                return true;
            },
        ],

        # 6 ------------------------------------------------------------------
        [
            'executables: demo + tests',
            [
                'cpp/src/main.cpp', 'cpp/tests/test_main.cpp',
                'cpp/CMakeLists.txt'
            ],
            [
'Builds ffi_bridge_demo and ffi_cpp_tests and links them against ffi_cpp and',
'libffi_rust.so; CMake also embeds a build-tree RUNPATH pointing at the .so.',
            ],
            sub {
                my ( $code, $out ) =
                  $cmake_build->( 'ffi_bridge_demo', 'ffi_cpp_tests' );
                show_output(
                    $out,
                    match => qr/Building CXX|Linking|Built target|rror|arning/,
                    limit => 10
                );
                return false if $code;
                my $ok = true;
                for my $exe ( $demo, $tests ) {
                    if ( -x $exe ) {
                        printf "  artifact: %s (%d bytes)\n", rel($exe),
                          -s $exe;
                    }
                    else {
                        say '  artifact: ' . rel($exe) . ' MISSING';
                        $ok = false;
                    }
                }
                return $ok;
            },
        ],

        # 7 ------------------------------------------------------------------
        [
            'loader: dynamic linking',
            [ 'cpp/CMakeLists.txt (BUILD_RPATH)', 'rust/build.rs (SONAME)' ],
            [
'At startup the dynamic loader reads NEEDED + RUNPATH from the executable and',
'resolves libffi_rust.so. A bare name in NEEDED is what makes it location-proof.',
            ],
            sub {
                my $ok = true;
                if ( defined find_in_path('readelf') ) {
                    my $dyn = capture( 'readelf', '-d', $demo ) // '';
                    my @needed =
                      $dyn =~ /\(NEEDED\)\s+Shared library:\s+\[([^\]]+)\]/g;
                    my ($runpath) = $dyn =~
/\((?:RUNPATH|RPATH)\)\s+Library (?:runpath|rpath):\s+\[([^\]]+)\]/;
                    say '  NEEDED  : ' . join( ', ', @needed );
                    say '  RUNPATH : ' . ( $runpath // 'none' );
                    unless ( grep { $_ eq 'libffi_rust.so' } @needed ) {
                        say
'  ! NEEDED lacks a bare "libffi_rust.so" (relative path recorded? SONAME missing?)';
                        $ok = false;
                    }
                    my $rust_dir =
                      File::Spec->catdir( $build, 'rust-target', $profile );
                    unless ( defined $runpath
                        && index( $runpath, $rust_dir ) >= 0 )
                    {
                        say "  ! RUNPATH does not contain $rust_dir";
                        $ok = false;
                    }
                }
                else {
                    say '  (readelf not found: NEEDED/RUNPATH check skipped)';
                }
                if ( defined find_in_path('ldd') ) {
                    my $ldd = capture( 'ldd', $demo ) // '';
                    my ($line) = $ldd =~ /^\s*(libffi_rust\.so\s+=>.*)$/m;
                    say '  ldd     : '
                      . ( $line // 'libffi_rust.so not listed' );
                    $ok = false if !defined $line || $line =~ /not found/;
                }
                else {
                    say '  (ldd not found: resolution check skipped)';
                }
                return $ok;
            },
        ],

        # 8 ------------------------------------------------------------------
        [
            'ctest: Rust + C++ tests',
            [
                'rust/tests/*.rs', 'cpp/tests/test_main.cpp',
                'rust/CMakeLists.txt (add_test)'
            ],
            [
'ctest runs two suites: "cargo test" (C-ABI, callbacks, dictionary vs source',
'file) and the C++ harness (ownership, borrowing, C++ vs Rust dictionary).',
            ],
            sub {
                my @cmd =
                  ( 'ctest', '--test-dir', $build, '--output-on-failure' );
                show_cmd(@cmd);
                my ( $code, $out ) = run_capture_in( undef, @cmd );
                show_output(
                    $out,
                    match =>
qr/Test\s+#|tests passed|tests failed|Total Test time|\*\*\*|FAILED/,
                    limit => 14
                );
                return $code == 0;
            },
        ],

        # 9 ------------------------------------------------------------------
        [
            'demo: final result',
            ['cpp/src/main.cpp'],
            [
'Runs the demo from a neutral directory. Every "[rust-log]" line is Rust calling',
'back into a C++ lambda; the [4xx] lines come from the error dictionary.',
            ],
            sub {
                my $cwd = File::Spec->tmpdir;
                say
"  cwd: $cwd (not the build dir: proves the loader does not depend on it)";

# Line-buffered stdout keeps stdout/stderr in their natural order inside the pipe.
                my @cmd =
                  defined find_in_path('stdbuf')
                  ? ( 'stdbuf', '-oL', $demo )
                  : ($demo);
                show_cmd(@cmd);
                my ( $code, $out ) = run_capture_in( $cwd, @cmd );
                show_output( $out, all => 1 );
                my $callbacks = () = $out =~ /\[rust-log:/g;
                say "  Rust -> C++ callbacks received: $callbacks";
                return $code == 0 && $callbacks > 0;
            },
        ],
    );

    my $total = scalar @steps;
    my $index = 0;
    my $stopped_at;
    for my $step (@steps) {
        $index++;
        my $result = trace_step( $index, $total, @$step );
        if ( !defined $result || !$result ) {
            $stopped_at = $index;
            last;
        }
    }

    if ( defined $stopped_at ) {
        for my $i ( $stopped_at .. $total - 1 ) {
            push @TRACE, [ $steps[$i][0], 'SKIP', 0 ] if $i >= scalar(@TRACE);
        }
    }

    say '';
    say '=' x 72;
    say 'SUMMARY';
    say '=' x 72;
    my ( $n, $time_sum, $failed ) = ( 0, 0, 0 );
    for my $row (@TRACE) {
        $n++;
        my ( $name, $status, $seconds ) = @$row;
        printf "  %d  %-30s %-5s %6.2fs\n", $n, $name, $status, $seconds;
        $time_sum += $seconds;
        $failed++ if $status eq 'FAIL';
    }
    printf "  total: %.2fs\n", $time_sum;

    my $all_ok = !$failed && !defined $stopped_at;
    say '';
    say $all_ok
      ? '[trace] pipeline completed: every stage produced the expected artifacts'
      : '[trace] pipeline stopped early: fix the failing step and run trace again';
    return $all_ok ? 0 : 1;
}

# ----------------------------------------------------------------------------
# Entry point
# ----------------------------------------------------------------------------

sub usage() {
    print <<~'EOT';
    Usage: manage.pl <command> [options] [-- extra args]

    Commands:
      setup    generate error dictionaries and configure CMake
      check    verify toolchain, project files and generated files
      build    configure (if needed) and build everything
      test     build and run all tests (ctest: cargo tests + C++ tests)
      run      run scripts/gen_errors.pl (pass its arguments after '--', e.g. -- --check)
      demo     build and run the C++ demo executable
      trace    walk the whole pipeline step by step, explaining and inspecting each stage
      clean    remove build artifacts (--all also removes generated files)

    Options:
      --build-dir DIR    build directory (default: <root>/build)
      --type TYPE        Release (default) or Debug
      --jobs N, -j N     parallel jobs (default: number of CPUs)
      --all              with 'clean': also remove generated files
      --fresh            with 'trace': start from scratch (build dir, rust/target, generated files)
      --pause            with 'trace': wait for Enter before each step (q quits)
      --verbose, -v      with 'trace': show full command output instead of the key lines
    EOT
    return;
}

my %COMMANDS = (
    setup => \&cmd_setup,
    check => \&cmd_check,
    build => \&cmd_build,
    test  => \&cmd_test,
    run   => \&cmd_run,
    demo  => \&cmd_demo,
    trace => \&cmd_trace,
    clean => \&cmd_clean,
);

sub main(@args) {

    if ($> == 0) {
        die "[manage] Error: This script cannot be run with root privileges for security reasons.\n";
    }

    my $command = shift(@args) // '';
    if ( $command eq '' || $command =~ /^(?:-h|--help|help)$/ ) {
        usage();
        return 0;
    }
    my $handler = $COMMANDS{$command};
    unless ($handler) {
        warn "[manage] unknown command '$command'\n";
        usage();
        return 2;
    }

    GetOptionsFromArray(
        \@args,
        'build-dir=s' => \$opt{build_dir},
        'type=s'      => \$opt{type},
        'jobs|j=i'    => \$opt{jobs},
        'all'         => \$opt{all},
        'fresh'       => \$opt{fresh},
        'pause'       => \$opt{pause},
        'verbose|v'   => \$opt{verbose},
    ) or return 2;

    $opt{build_dir} = File::Spec->rel2abs( $opt{build_dir} );
    unless ( $opt{type} =~ /^(?:Release|Debug)$/ ) {
        warn "[manage] --type must be Release or Debug\n";
        return 2;
    }

    my $rc = eval { $handler->(@args) };
    if ($@) {
        print STDERR $@;
        return 1;
    }
    return $rc // 0;
}

exit main(@ARGV);
