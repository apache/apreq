use ExtUtils::MakeMaker;

use 5.005;

use Apache::Test5005compat;

use Apache::TestMM qw(test clean);
use Apache::TestReport ();
use Apache::TestSmoke ();
use Apache::TestRun ();

use File::Find qw(finddepth);
use Apache::TestTrace;

use Apache::Test;
use Apache::TestConfigC;
use base qw/Apache::TestConfig/;


sub cmodules_write_makefile {
    my($self, $mod) = @_;

    my $dversion = $self->server->dversion;
    my $name = $mod->{name};
    my $makefile = "$mod->{dir}/Makefile";
    debug "WRITING $makefile";

    my $lib = $self->cmodules_build_so($name);

    my $fh = Symbol::gensym();
    open $fh, ">$makefile" or die "open $makefile: $!";

    print $fh <<EOF;
APXS=$self->{APXS}
all: $lib

$lib: $name.c
	\$(APXS) -L../../../src -I../../../src -lapreq $dversion -I$self->{cmodules_dir} -c $name.c

clean:
	-rm -rf $name.o $name.lo $name.slo $name.la .libs
EOF

    close $fh or die "close $makefile: $!";
}


$Apache::TestTrace::Level = 'debug';
bless my $cfg = Apache::Test->config();

unless (-d "t/conf") {
    warn "Creating t/conf directory.";
    mkdir "t/conf" or die "mkdir 't/conf' failed: $!";
}

$cfg->preamble(LoadModule => [apreq_module => "../.libs/mod_apreq.so"]);
$cfg->cmodules_configure;
$cfg->generate_httpd_conf;

Apache::TestMM::filter_args();
Apache::TestRun->generate_script;

