use ExtUtils::MakeMaker;
use strict;
use FindBin;
use 5.005;

use File::Path qw(mkpath);
use lib qw(Apache-Test/lib);

use constant HAS_APACHE_TEST => eval {require Apache::Test};
# enable 'make test|clean'
if (HAS_APACHE_TEST) {
    require Apache::TestMM;
    Apache::TestMM->import(qw(test clean));
}
BEGIN {
    unless (eval {require mod_perl}) {
        die "Please install mod_perl: 1.25 < version < 1.99\n($@)";
    }
    if ($mod_perl::VERSION < 1.2402) {
        die "Please upgrade mod_perl to 1.24_02 or greater";
    }
    elsif ($mod_perl::VERSION > 1.98) {
        die "mod_perl 1.x ( < 1.99) is required";
    }
}

my $Is_Win32 = ($^O eq "MSWin32") || $ENV{WIN32_TEST};

my $conf_data = join '', <DATA>;
$conf_data =~ s|(blib/arch)|$FindBin::Bin/$1|;

if (not $Is_Win32) {
  eval {
    require Apache::Test;
    # accept the configs from command line
    Apache::TestMM::filter_args();
    Apache::TestMM::generate_script('t/TEST');
#    local $SIG{ALRM} = sub { die "alarm\n" };
#    alarm 60;
#    my %params = Apache::test->get_test_params();
#    alarm 0;

#    Apache::test->write_httpd_conf(%params, include => $conf_data);
#    *MY::test = sub {Apache::test->MM_test(%params)};
  };
  print "\nskipping test setup...$@" if $@;
}
else {
  write_win32_conf();
}

# prerequisites
my %require = (
    "Apache::Test" => "", # any version will do
);

my $myVERSION = MM->parse_version('Request/Request.pm');
$myVERSION =~ s/(\d\d)(\d\d)$/$1_$2/;
my $lib;
check_compat();

if ($Is_Win32) {
  require Cwd;
  my $cwd = Cwd::cwd();
  $cwd =~ s#\\#/#g;
  my $root = "$cwd/blib/arch/auto";
  mkpath $root, 1, 0755 unless -d $root;
  mkpath "$root/libapreq", 1, 0755 unless -d "$root/libapreq";
  $lib = "$root/libapreq/libapreq.lib";
  open (FILE, ">>$lib") 
    or die "Cannot touch $lib: $!\n";
  close FILE;  
  require Apache::MyConfig;
  $ENV{APREQ_LIB} = "$root/libapreq";
  $ENV{AP_INC} = $Apache::MyConfig::Setup{APACHE_INC};
  $ENV{MP_INC} = $Apache::MyConfig::Setup{MODPERL_INC};
  $ENV{AP_LIB} = $Apache::MyConfig::Setup{APACHE_LIB};
  $ENV{MP_LIB} = $Apache::MyConfig::Setup{MODPERL_LIB};
  unless (-f "$ENV{MP_LIB}/mod_perl.so") {
    die "Cannot find mod_perl.so in $ENV{MP_LIB}. " .
        "Please build mod_perl first";
  }
  unless (-f "$ENV{AP_LIB}/ApacheCore.lib") {
    die "Cannot find ApacheCore.lib in $ENV{AP_LIB}. " .
        "Please build apache first.";
  }
}

WriteMakefile(
	      'NAME'	 => 'libapreq',
	      'VERSION' => $myVERSION,
	      'DIR' => [qw(c Request Cookie)], 
              PREREQ_PM => \%require,
              clean     => {
                            FILES => "@{ clean_files() }",
                           },
	      );

sub clean_files {
    return [qw( t/TEST t/httpd.conf t/httpd.loc t/error_log t/httpd )];
}

if ($Is_Win32 and -e $lib) {
  unlink $lib or die "Cannot unlink $lib: $!\n";
}

sub check_compat {
    eval { require Apache::Request };
    return if $@;
    my $v = $Apache::Request::VERSION;
    return if $v >= 0.2000;
    print "libapreq is not compatible with  Apache::Request version $v\n";
    my $ans = prompt("Continue?", "n");
    if ($ans =~ /^n/i) {
	print "Aborting.\n";
	exit;
    }
}

sub MY::pasthru {
     my $self = shift;
    chomp(my $str = $self->MM::pasthru);
    join $/, "$str\\", 
     "\t".'DEFINE="$(DEFINE)"',
    "";
}
sub MY::postamble { 
    return <<'EOF'; 

cvs_tag :
	cvs tag v$(VERSION_SYM) . 
	@echo update the VERSION in Apache/Request.pm now 

EOF
}

sub write_win32_conf {
  use Cwd;
  use Config;
  my $dll;
  (my $libperl = $Config{libperl}) =~ s!(.*)\.lib!$Config{bin}/$1.dll!;
  $libperl =~ s!\\!/!g;
  $libperl = undef unless -e $libperl;
 SEARCH: {
    for my $drive ('C'..'G') {
      for my $p ("program files/apache", "apache") { 
	last SEARCH if -e ($dll = "$drive:/$p/modules/mod_perl.so");
      }
    }
  }
  unless (-e $dll) {
    require ExtUtils::MakeMaker;
    ExtUtils::MakeMaker->import('prompt');
    $dll = prompt("Where is your mod_perl.so located ?", $dll);
  }
  
  die "Can't find mod_perl.so!" unless -e $dll;

  (my $mime = $dll) =~ s!modules/mod_perl.so!conf/mime.types!;
  unless (-e $mime) {
    require ExtUtils::MakeMaker;
    ExtUtils::MakeMaker->import('prompt');
    $dll = prompt("Where is your mime.types located ?", $mime);
  }
  die "Can't find mime.types!" unless -e $mime;
  
  my $cwd = cwd;
  open(ORIG, 't/httpd.conf.win32') or die "Cannot open t/httpd.conf.win32: $!";
  open(CONF, '>t/httpd.conf') or die "Cannot open t/httpd.conf: $!";
  while (<ORIG>) {
    s/CWD/$cwd/g;
    s/MIME/"$mime"/;
    if ($libperl) {
      s/(LoadModule perl_module )DLL/LoadFile "$libperl"\n$1"$dll"/;
    }
    else {
      s/DLL/$dll/;
    }
    print CONF $_;
  }
  close ORIG;
  print CONF $conf_data;
  close CONF;
  *MY::test = sub {
return << 'EOF';
test:	run_tests

run_tests:
        $(FULLPERL) -I"blib/lib" -I"blib/arch" t/TEST.win32

EOF
};

}

__DATA__
<Perl>
   unshift @INC, "blib/arch";
</Perl>
PerlModule Apache::PerlRun
PerlModule Apache::Request
PerlModule Apache::Cookie

<FilesMatch "\.pl">
    SetHandler perl-script
    PerlHandler Apache::PerlRun
    Options +ExecCGI
</FilesMatch>











