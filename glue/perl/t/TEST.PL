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
    bless my $cfg = $self->{test_config}, "My::TestConfig";
    $cfg->preamble(IfModule => '!mod_apreq.c',
               qq(LoadModule apreq_module "$env_dir_libs/mod_apreq.so"\n));
    $self->SUPER::configure_modperl();
}


package My::TestConfig;
use base 'Apache::TestConfig';
use File::Spec::Functions qw(catfile);
sub configure_startup_pl {
    my $self = shift;

    #for 2.0 we could just use PerlSwitches -Mlib=...
    #but this will work for both 2.0 and 1.xx
    if (my $inc = $self->{inc}) {
        my $include_pl = catfile $self->{vars}->{t_conf}, 'modperl_inc.pl';
        my $fh = $self->genfile($include_pl);
        # make sure that the dev libs come before blib
        my $fixup = Apache::TestConfig->modperl_2_inc_fixup();
        print $fh $fixup;
        for (reverse @$inc) {
            print $fh "use lib '$_';\n";
        }
        print $fh "1;\n";
    }

    if ($self->server->{rev} >= 2) {
        $self->postamble(PerlSwitches => "-Mlib=$self->{vars}->{serverroot}");
    }

    my $startup_pl = catfile $self->{vars}->{t_conf}, 'modperl_startup.pl';


    unless (-e $startup_pl) {
        my $fh = $self->genfile($startup_pl);
        print $fh $self->startup_pl_code;
        close $fh;
    }

    $self->postamble(PerlRequire => $startup_pl);
}

