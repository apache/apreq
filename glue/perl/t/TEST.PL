#!perl

use strict;
use warnings FATAL => 'all';

use lib qw(lib Apache-Test/lib);

use Cwd;
cwd =~ m{^(.+httpd-apreq-2)} or die "Can't find base cvs directory";
my $base_dir = $1;
my $env_dir = "$base_dir/env";

use base 'Apache::TestRunPerl';

main::->new->run(@ARGV);

sub configure_modperl {
    my $self = shift;
    my $cfg = $self->{test_config};
    $cfg->preamble(IfModule => '!mod_apreq.c',
               qq(LoadModule apreq_module "$env_dir/.libs/mod_apreq.so"\n));
    $self->SUPER::configure_modperl();
}
