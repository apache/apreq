#!perl

use strict;
use warnings FATAL => 'all';

use lib qw(lib Apache-Test/lib);

use Apache2;
use Apache::Build;
require Win32 if Apache::Build::WIN32;

use Cwd;
my $cwd = Apache::Build::WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;
$cwd =~ m{^(.+httpd-apreq-2)} or die "Can't find base cvs directory";
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
}

sub configure_modperl {
    my $self = shift;
    my $cfg = $self->{test_config};
    $cfg->preamble(IfModule => '!mod_apreq.c',
               qq(LoadModule apreq_module "$env_dir_libs/mod_apreq.so"\n));
    $self->SUPER::configure_modperl();
}
