use ExtUtils::MakeMaker;
use Config;
use strict;
use lib qw(../lib);
use Apache::libapreq qw(xsubpp);

my $Is_Win32 = ($^O eq "MSWin32") || $ENV{WIN32_TEST}; 
my @mm_args = (
	       'NAME'	=> 'Apache::Cookie',
	       'VERSION_FROM' => 'Cookie.pm',
	       );

if ($Is_Win32) {
    win32_setup();
    xsubpp("Cookie");
}
else {
    require Apache::src;
    my $src = Apache::src->new;
    use File::Path 'mkpath';
    my $root = "../blib/arch/auto/libapreq";
    mkpath $root, 1, 0755 unless -d $root;

    #grr, must trick ExtUtils::Liblist
    system "touch $root/libapreq.a";
    WriteMakefile(
        @mm_args,
        'INC'	=> "-I../c ". $src->inc,
        'TYPEMAPS'  => $src->typemaps,
        'LIBS' => "-L$root -lapreq",
	'dynamic_lib' => {
		'OTHERLDFLAGS' => $src->otherldflags,
	},
    );
    unlink "$root/libapreq.a";
}

sub win32_setup {
    *MY::dynamic = sub {
	my $self = shift;
	my $string = $self->MM::dynamic;
	$string =~ s/(Makefile\s+).*/$1/g;
	$string;
    };
    push @mm_args, SKIP => [qw(dynamic_bs dynamic_lib)];
    WriteMakefile(@mm_args);
}
