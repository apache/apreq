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
    DIR => [qw(xs)],
    clean => { FILES => "xs t/logs t/TEST @scripts" },
    realclean => { FILES => "xsbuilder/tables" },
);


sub copy_docs {
    my @lines;
    my $dfs = '$(DIRFILESEP)';

    foreach my $file (@_) {
        my @dirs;
        $file =~ /(\w+\.pod)$/ or next;
        if ($1 eq "Error.pod" or $1 eq "Table.pod") {
            push @dirs, "Apache$dfs$_" for qw/Request Cookie Upload/;
            push @dirs, join $dfs, qw/Apache Cookie Jar/ if $1 eq "Error.pod";
        }
        else {
            push @dirs, "Apache";
        }
        push @lines, map <<EOT, @dirs;
subdirs :: \$(INST_LIBDIR)$dfs$_$dfs$1

\$(INST_LIBDIR)$dfs$_$dfs$1: $file
	\$(NOECHO) \$(MKPATH) \$(INST_LIBDIR)$dfs$_
	\$(CP) $file \$(INST_LIBDIR)$dfs$_$dfs$1

EOT
    }
    return join "", @lines;
}

sub MY::postamble {
    my @docs = <docs/*.pod>;
    my @tests = @docs;
    s/pod$/t/ for @tests;

    return copy_docs(@docs) . sprintf <<'EOT', "@tests";
%%.t : %%.pod
	pod2test $< $@

doc_test : %s
	$(FULLPERL) -Mblib -MTest::Harness -e 'runtests(@ARGV)' $^

test :: doc_test

EOT

}

__END__
