#!/usr/bin/env perl
# ---------------------------------------------------------------------------
# generate_project.pl
#
# Creates the on-disk directory layout for the "mini_db_engine" project
# (src/, test/, dev/) and then delegates to init_project_files.pl, which
# populates those directories with minimal stub files sufficient to
# configure and build the CMake project end to end.
#
# Usage:
#   perl generate_project.pl [base_dir]
#
#   base_dir  Optional. Directory under which "mini_db_engine/" is created.
#             Defaults to the current working directory.
# ---------------------------------------------------------------------------
use v5.40;

use File::Spec;
use File::Path qw(make_path);
use FindBin    qw($RealBin);

main();

sub main {
    guard_against_root();

    my $project_name = 'mini_db_engine';
    my $base_dir     = $ARGV[0] // '.';
    my $project_root = File::Spec->catdir( $base_dir, $project_name );

    say "==> Generating hierarchy for '$project_name'";
    say "==> Project root: $project_root";

    create_directories($project_root);
    invoke_init_script($project_root);

    say "==> Done. Scaffold ready at: $project_root";

    return 0;
}

# Creates the top-level project directory plus src/, test/ and dev/.
sub create_directories ($project_root) {
    my @dirs = (
        $project_root,
        File::Spec->catdir( $project_root, 'src' ),
        File::Spec->catdir( $project_root, 'test' ),
        File::Spec->catdir( $project_root, 'dev' ),
    );

    for my $dir (@dirs) {
        make_path( $dir, { verbose => 1 } );
    }

    return;
}

# Locates init_project_files.pl next to this script (regardless of the
# caller's current working directory) and runs it against the freshly
# created project root.
sub invoke_init_script ($project_root) {
    my $init_script = File::Spec->catfile( $RealBin, 'init_project_files.pl' );

    die "Companion script not found: $init_script\n" unless -f $init_script;

    say "==> Delegating file initialization to: $init_script";

    my $exit_code = system( $^X, $init_script, $project_root );
    die "init_project_files.pl failed (exit code $exit_code)\n"
      if $exit_code != 0;

    return;
}

# Refuses to proceed when run as root (real or effective uid 0), either
# directly or via sudo/su. Scaffolding a project as root creates
# root-owned files in what is normally a regular user's workspace, which
# is almost never what you want and easy to do by accident.
sub guard_against_root {
    if ( $< == 0 || $> == 0 ) {
        die "Refusing to run as root (uid 0). Re-run as a regular user.\n";
    }

    return;
}
