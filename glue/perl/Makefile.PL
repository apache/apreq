use Apache2;
use ModPerl::MM;

use 5.005;

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
    my $stat = do "../../build/xsbuilder.pl";
    die "failed to run xsbuilder: $@" if !defined($stat) && $@;
}

finddepth(sub {
    return unless /(.*?\.pl)\.PL$/;
    push @scripts, "$File::Find::dir/$1";
}, '.');

Apache::TestMM::filter_args();
Apache::TestMM::generate_script("t/TEST");

ModPerl::MM::WriteMakefile(
    NAME => 'libapreq2',
    VERSION => '0.1',
    DIR => [qw(xs)],
    clean => { DIRS => "blib", FILES => "t/TEST @scripts" },
);

__END__
