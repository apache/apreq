use ExtUtils::MakeMaker;

use 5.005;

use lib qw(/home/joe/src/apache/httpd-test/perl-framework/Apache-Test/lib);

use Apache::Test5005compat;

use Apache::TestMM qw(test clean);
use Apache::TestReport ();
use Apache::TestSmoke ();
use Apache::TestRun ();
use Apache::TestConfigPerl ();
use File::Find qw(finddepth);
use Cwd;
cwd =~ m{^(.+httpd-apreq-2)} or die "Can't find base cvs directory";
my $base_dir = $1;
my $src_dir = "$base_dir/src";
my $env_dir = "$base_dir/env";
my @scripts = ();

system "../../build/xsbuilder.pl VERSION run";

finddepth(sub {
    return unless /(.*?\.pl)\.PL$/;
    push @scripts, "$File::Find::dir/$1";
}, '.');

Apache::TestMM::filter_args();

Apache::TestMM::generate_script("t/TEST");
#for my $script (@scripts) {
#    Apache::TestMM::generate_script($script);
#}


#for my $util (qw(Report Smoke Run)) {
#    my $class = "Apache::Test${util}";
#    $class->generate_script;
#}

WriteMakefile(
    NAME => 'httpd-apreq-2',
    VERSION => '0.01',
    DIR => [qw(xs)],
    clean => { DIRS => "blib", FILES => "@scripts" },
);

__END__
$Apache::TestTrace::Level = 'debug';
my $cfg = Apache::Test->config();

$cfg->preamble(IfModule => '!mod_apreq.c', 
               qq(LoadModule apreq_module "$env_dir/.libs/mod_apreq.so"\n));
$cfg->configure_httpd;
$cfg->configure_libmodperl;
$cfg->configure_startup_pl;
$cfg->save; # GRR! why doesn't this write the config file!?!?!?
#$cfg->modules_configure_pm_tests;
$cfg->generate_httpd_conf;

__DATA__
