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
my @scripts = ();

{
    local @ARGV = qw(VERSION run);
    print "Building xs/  ...\n";
    do "../../build/xsbuilder.pl";
}

finddepth(sub {
    return unless /(.*?\.pl)\.PL$/;
    push @scripts, "$File::Find::dir/$1";
}, '.');

Apache::TestMM::filter_args();
Apache::TestMM::generate_script("t/TEST");

WriteMakefile(
    NAME => 'httpd-apreq-2',
    VERSION => '0.01',
    DIR => [qw(xs)],
    clean => { DIRS => "blib", FILES => "@scripts" },
);

__END__
