use ExtUtils::MakeMaker;

use 5.005;

use lib qw(/home/joe/src/apache/httpd-test/perl-framework/Apache-Test/lib);

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

$cfg->preamble(LoadModule => [apreq_module => "../.libs/mod_apreq.so"]);
$cfg->cmodules_configure;
$cfg->generate_httpd_conf;

my @scripts = ();

finddepth(sub {
    return unless /(.*?\.pl)\.PL$/;
    push @scripts, "$File::Find::dir/$1";
}, '.');

Apache::TestMM::filter_args();

for my $script (@scripts) {
    Apache::TestMM::generate_script($script);
}

for my $util (qw(Report Smoke Run)) {
    my $class = "Apache::Test${util}";
    $class->generate_script;
}
