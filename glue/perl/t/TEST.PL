#!perl

use strict;
use warnings FATAL => 'all';
use Apache2;
use Apache::Build;
use constant WIN32 => Apache::Build::WIN32;

use Cwd;
my $cwd = WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;
$cwd =~ m{^(.+)/glue/perl$} or die "Can't find base cvs directory";
my $base_dir = $1;
my $env_dir = "$base_dir/env";
my $env_dir_libs = Apache::Build::WIN32 ?
    "$base_dir/win32/libs" : "$env_dir/.libs";

use base 'Apache::TestRunPerl';

main::->new->run(@ARGV);

sub pre_configure {
    my $self = shift;
    # Don't load an installed mod_apreq
    Apache::TestConfig::autoconfig_skip_module_add('mod_apreq.c');
    $self->SUPER::pre_configure();
}

sub configure_modperl {
    my $self = shift;
    my $cfg = $self->{test_config};
    # make sure Win32 uses the just-built libapreq2.dll
    if (WIN32) {
        $cfg->preamble(qq{LoadFile "$env_dir_libs/libapreq2.dll"\n});
    }
    $cfg->preamble(IfModule => '!mod_apreq.c',
               qq(LoadModule apreq_module "$env_dir_libs/mod_apreq.so"\n));
    $self->SUPER::configure_modperl();
}
