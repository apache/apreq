use ExtUtils::MakeMaker;
use strict;

my $myVERSION = MM->parse_version('Request/Request.pm');
$myVERSION =~ s/(\d\d)(\d\d)$/$1_$2/;

check_compat();

WriteMakefile(
	      'NAME'	 => 'libapreq',
	      'VERSION' => $myVERSION,
	      'DIR' => [qw(c Request Cookie)], 
	      'macro' => {
		  CVSROOT => 'modperl.com:/local/cvs_repository',
	      },
	      );

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
	cvs -d $(CVSROOT) tag v$(VERSION_SYM) . 
	@echo update Apache/Request.pm's VERSION now 

EOF
} 

