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

Apache::TestMM::filter_args();
Apache::TestMM::generate_script("t/TEST");

$Apache::TestTrace::Level = 'debug';

bless my $cfg = Apache::Test->config();
$cfg->cmodules_configure;
