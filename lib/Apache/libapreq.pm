package Apache::libapreq;
$VERSION = '1.2';

use strict;
use Config;
use Exporter ();
{
    no strict;
    @ISA = qw(Exporter);
    @EXPORT = qw(ccopts ldopts xsubpp);
}

sub ccopts {
    print " -I$Config{installsitearch}/auto/libapreq/include ";
}

sub ldopts {
    print " -L$Config{installsitearch}/auto/libapreq -lapreq ";
}

sub xsubpp {
    my $name = shift;
    my $lib = $Config{privlibexp}; 
    my $xsubpp = "$^X $lib/ExtUtils/xsubpp";
    my @typemaps = (
		    -typemap => "$lib/ExtUtils/typemap",
		    -typemap => "../typemap",
		    );
    system "$xsubpp @typemaps $name.xs > $name.c"; 
}

1;
__END__

=head1 NAME

Apache::libapreq - Generate compiler and linker flags for libapreq

=head1 SYNOPSIS

 #Makefile
 CCOPTS = `perl -MApache::libapreq -e ccopts`
 LDOPTS = `perl -MApache::libapreq -e ldopts`

=head1 DESCRIPTION

The I<libapreq> package installs libraries and include files into Perl
architecture dependent locations.

=head1 FUNCTIONS

=over 4

=item ccopts

Simply invokes: 

    print "-I$Config{installsitearch}/auto/libapreq/include"

=item ldopts

Simply invokes: 

    print "-L$Config{installsitearch}/auto/libapreq -lapreq"

=back

=head1 AUTHOR

Doug MacEachern
