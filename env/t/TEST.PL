use strict;
use warnings FATAL => 'all';

use constant WIN32 => $^O eq 'MSWin32';
require Win32 if WIN32;

use Cwd;
my $cwd = WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;
$cwd =~ m{^(.+)/env$} or die "Can't find base directory";
my $base_dir = $1;
my $env_dir = "$base_dir/env";
my $env_dir_libs = WIN32 ?
    "$base_dir/win32/libs" : "$env_dir/.libs";

use base 'Apache::TestRun';
$Apache::TestTrace::Level = 'debug';
main::->new->run(@ARGV);

sub pre_configure {
    my $self = shift;
    # Don't load an installed mod_apreq
    Apache::TestConfig::autoconfig_skip_module_add('mod_apreq.c');
}

sub configure {
    my $self = shift;
    my $cfg = $self->{test_config};
    $cfg->preamble(IfModule => '!mod_apreq.c',
               qq(LoadModule apreq_module "$env_dir_libs/mod_apreq.so"\n));
    bless $cfg, "My::TestConfig";
    $self->SUPER::configure();
}



package My::TestConfig;
use Apache::TestTrace;
use Apache::TestConfigC;
use Config;
use constant WIN32 => $^O eq 'MSWin32';

use base 'Apache::TestConfig';

sub cmodules_httpd_conf {
    my $self = shift;
    debug "WRITING httpd.conf with " . join", ",map $_->{name},
        @{$self->{cmodules}};
    $self->SUPER::cmodules_httpd_conf(@_);
}
sub cmodules_write_makefile {
    my($self, $mod) = @_;
    my $dversion = $self->server->dversion;
    my $name = $mod->{name};
    my $makefile = "$mod->{dir}/Makefile";
    debug "WRITING $makefile for $name";
    my $lib = $self->cmodules_build_so($name);

    my $fh = Symbol::gensym();
    open $fh, ">$makefile" or die "open $makefile: $!";

    if (WIN32) {
        my @goners = map {$name . '.' . $_} qw(exp ilk lib pdb so lo);
        print $fh <<EOF;
APXS=$self->{APXS}
all: $lib

$lib: $name.c
        \$(APXS) -L../../../win32/libs -I../../../src -llibapreq2 -llibhttpd $dversion -p -I$self->{cmodules_dir} -c $name.c

clean:
	-erase @goners vc60.pdb 
EOF
    }
    else {
        print $fh <<EOF;
APXS=$self->{APXS}
all: $lib

$lib: $name.c
	\$(APXS) -L../../../src -I../../../src -lapreq2 $dversion -I$self->{cmodules_dir} -c $name.c

clean:
	-rm -rf $name.o $name.lo $name.slo $name.la .libs
EOF
    }
    close $fh or die "close $makefile: $!";
}

sub cmodules_makefile_vars {
    my $make = $ENV{MAKE} || $Config{make};
    return <<EOF; # XXX: do we need to propagate all the vars in config.nice?
MAKE=$make
EOF
}

sub cmodules_make {
     my $self = shift;
     my $targ = shift || 'all';
     my $make = $ENV{MAKE} || $Config{make};

     my $cmd = "cd $self->{cmodules_dir} && $make $targ";
     debug $cmd;
     system $cmd;
     if ($?) {
         die "Failed to build c-modules";
     }
}
