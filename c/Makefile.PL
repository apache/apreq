use ExtUtils::MakeMaker;
use Config;

my $Is_Win32 = ($^O eq "MSWin32") || $ENV{WIN32_TEST}; 

if ($Is_Win32) {
    win32_setup();
}
else {
    require Apache::src;
    my $src = Apache::src->new;

    use Cwd;
    my $pwd = fastcwd;
    my @objs = qw(apache_request.o apache_cookie.o multipart_buffer.o);
    $MY_LD_RUN_PATH = "$Config{installsitearch}/auto/libapreq:$pwd"; 

    WriteMakefile(
       #grr, problems with things finding libapreq.so, sort out later.
       'LINKTYPE' => 'static',
#       'SKIP' => [qw(dynamic_lib dynamic_bs)],
       'NAME'	 => 'libapreq',
       'INC' 	 => $src->inc,
       'TYPEMAPS'   => $src->typemaps,
       'OBJECT' => "@objs",
    );
}

use Cwd;
my $pwd = fastcwd;
$ENV{LD_RUN_PATH} = 
    "$ENV{LD_RUN_PATH}:$Config{installsitearch}/auto/libapr:$pwd";

sub MY::dynamic {
    my $self = shift;
    my $string = $self->MM::dynamic;
    $string =~ s/(Makefile\s+).*/$1/g;
    $string;
}

sub MY::top_targets { 
    my $self = shift; 
    my $string = $self->MM::top_targets; 
    my $libapreq = <<"EOF";
LD_RUN_PATH = $ENV{LD_RUN_PATH}
EOF

    my $so = "\n".'$(DLBASE).$(DLEXT): $(INST_DYNAMIC)'."\n";

    $string =~ s/(pure_all\s+::\s+)(.*)/$1 static $2/; 
    return $libapreq . $so . $string;
}

sub MY::post_initialize { 
    my $self = shift; 
    my $to   = '$(INST_ARCHLIB)/' . "auto/libapreq/include"; 
    for (qw(apache_request apache_cookie multipart_buffer)) {
	$self->{PM}->{"$_.h"} = "$to/$_.h";
    }
    '';
} 

sub win32_setup {
    no_MY();
    push @mm_args, SKIP => [qw(dynamic_bs dynamic_lib)];
    WriteMakefile(@mm_args);
}

sub no_MY {
    for (keys %MY::) {
	if (defined &{ $MY::{$_} }) {
	    next if $_ eq 'dynamic';
	    undef &{ $MY::{$_} };
	    delete $MY::{$_};
	} 
    }
}
