use ExtUtils::MakeMaker;
use strict;
use 5.005;

my $myVERSION = MM->parse_version('Request/Request.pm');
$myVERSION =~ s/(\d\d)(\d\d)$/$1_$2/;
my $Is_Win32 = ($^O eq "MSWin32") || $ENV{WIN32_TEST}; 
my $lib;
check_compat();

if ($Is_Win32) {
  use mod_perl;
  unless ($mod_perl::VERSION >= 1.2402) {
    die "Please upgrade mod_perl to 1.24_02 or greater";
  }
  use Cwd;
  my $cwd = cwd;
  $cwd =~ s !\\!/!g;
  my $root = "$cwd/blib/arch/auto";
  use File::Path qw(mkpath);
  mkpath $root, 1, 0755 unless -d $root;
  mkpath "$root/libapreq", 1, 0755 unless -d "$root/libapreq";
  $lib = "$root/libapreq/libapreq.lib";
  open (FILE, ">>$lib") 
    or die "Cannot touch $lib: $!\n";
  close FILE;  
  use Apache::MyConfig;
  $ENV{APREQ_LIB} = "$root/libapreq";
  $ENV{AP_INC} = $Apache::MyConfig::Setup{APACHE_INC};
  $ENV{MP_INC} = $Apache::MyConfig::Setup{MODPERL_INC};
  $ENV{AP_LIB} = $Apache::MyConfig::Setup{APACHE_LIB};
  $ENV{MP_LIB} = $Apache::MyConfig::Setup{MODPERL_LIB};
  unless (-d $ENV{MP_INC} and -f "$ENV{MP_LIB}/mod_perl.so") {
    die "Please build mod_perl first";
  }
  unless (-d $ENV{AP_INC} and -f "$ENV{AP_LIB}/ApacheCore.lib") {
    die "Please build Apache first";
  }
}

WriteMakefile(
	      'NAME'	 => 'libapreq',
	      'VERSION' => $myVERSION,
	      'DIR' => [qw(c Request Cookie)], 
	      );

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

