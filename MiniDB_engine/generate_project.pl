#!/usr/bin/env perl
# ---------------------------------------------------------------------------
# generate_project.pl
#
# Uso:
#    perl generate_project.pl [base-dir]
#
#    base_dir   Optional. Directory under which "mini_db_engine/" is created.
#               Defaults to the current working directory.
# ---------------------------------------------------------------------------
use v5.40;

use File::Spec;
use File::Path qw(make_path);
use FindBin    qw($RealBin);
use POSIX      qw(strftime);

main();

sub main {
    guard_against_root();

    my $project_name = 'mini_db_engine';
    my $base_dir     = $ARGV[0] // '.';
    my $project_root = File::Spec->catdir( $base_dir, $project_name );

    logger( "info", "Generating hierarchy for '$project_name'" );
    logger( "info", "Project root : $project_root" );

    create_dirs($project_root);
    invoke_init_script($project_root);

    logger( "info", "Done." );

    return 0;
}

sub guard_against_root {
    if ( $< == 0 || $> == 0 ) {
        logger( "error",
            "Refusing to run as root (uid 0). Re-run as a regular user." );
    }
}

sub logger ( $level, $message ) {
    $level = uc($level);

    my $timestamp = strftime( "%Y-%m-%d %H:%M:%S", localtime );
    my $log       = "[$timestamp] [$level] $message";

    if ( $level eq "ERROR" ) {
        say STDERR $log;
        exit 1;
    }

    say $log;
}

sub create_dirs ($project_root) {
    my @dirs = (
        $project_root,
        File::Spec->catdir( $project_root, 'src' ),
        File::Spec->catdir( $project_root, 'dev' ),
    );

    for my $dir (@dirs) {
        logger( "Info", "mkdir $dir" );
        make_path($dir);
    }

    return;
}

sub invoke_init_script ($project_root) {
    my $init_script = File::Spec->catfile( $RealBin, 'init_project_files.pl' );

    if ( !-f $init_script ) {
        logger( "error", "Companion script not found: $init_script" );
    }

    logger( "info", "Delegating file initialization to: $init_script" );

    my $exit_code = system( $^X, $init_script, $project_root );
    if ( $exit_code != 0 ) {
        logger( "error",
            "init_project_files.pl failed (exit code $exit_code)" );
    }

    return;
}
