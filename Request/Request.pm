package Apache::Request;

use strict;
use mod_perl 1.17_01;
use Apache::Table ();

{
    no strict;
    $VERSION = '0.3003';
    @ISA = qw(Apache);
    __PACKAGE__->mod_perl::boot($VERSION);
}

#just prototype methods here, moving to xs later
sub param {
    my $self = shift;
    my($name, $value) = @_;
    my $tab = $self->parms;
    unless ($name) {
	my %seen;
	return wantarray ? grep { !$seen{$_}++ } keys %$tab : $tab;
    }
    if (defined $value) {
	$tab->set($name, $value);
    }
    return wantarray ? ($tab->get($name)) : scalar $tab->get($name);
}

1;
__END__

=head1 NAME

Apache::Request - Methods for dealing with client request data

=head1 SYNOPSIS

    use Apache::Request ();
    my $apr = Apache::Request->new($r);

=head1 DESCRIPTION

I<Apache::Request> is a subclass of the I<Apache> class, which adds methods
for parsing B<GET> requests and B<POST> requests where I<Content-type>
is one of I<application/x-www-form-urlencoded> or 
I<multipart/form-data>.

=head1 Apache::Request METHODS

=over 4

=item new

Create a new I<Apache::Request> object with an I<Apache> request_rec object:

    my $apr = Apache::Request->new($r);

All methods from the I<Apache> class are inherited.

The following attributes are optional:

=over 4

=item POST_MAX

Limit the size of POST data.  I<Apache::Request::parse> will return an
error code if the size is exceeded:

 my $apr = Apache::Request->new($r, POST_MAX => 1024);
 my $status = $apr->parse;

 if ($status) {
     my $errmsg = $apr->notes("error-notes");
     ...
     return $status;
 }

=item DISABLE_UPLOADS

Disable file uploads.  I<Apache::Request::parse> will return an
error code if a file upload is attempted:

 my $apr = Apache::Request->new($r, DISABLE_UPLOADS => 1);
 my $status = $apr->parse;

 if ($status) {
     my $errmsg = $apr->notes("error-notes");
     ...
     return $status;
 }

=back

=item parse

The I<parse> method does the actual work of parsing the request.
It is called for you by the accessor methods, so it is not required but
can be useful to provide a more user-friendly message should an error 
occur:
 
    my $r = shift;
    my $apr = Apache::Request->new($r); 
 
    my $status = $apr->parse; 
    unless ($status == OK) { 
	$apr->custom_response($status, $apr->notes("error-notes")); 
	return $status; 
    } 

=item param

Get or set request parameters:

    my $value = $apr->param('foo');
    my @values = $apr->param('foo');
    my @params = $apr->param;
    $apr->param('foo' => [qw(one two three)]);

=item upload

Returns a single I<Apache::Upload> object in a scalar context or
all I<Apache::Upload> objects in an array context: 

    my $upload = $apr->upload;
    my $fh = $upload->fh;
    my $lines = 0; 
    while(<$fh>) { 
        ++$lines; 
        ...
    } 

An optional name parameter can be passed to return the I<Apache::Upload>
object associated with the given name:

    my $upload = $apr->upload($name);

=back

=head1 Apache::Upload METHODS

=over 4

=item name

The name of the filefield parameter:

    my $name = $upload->name;

=item filename

The filename of the uploaded file:

    my $filename = $upload->filename;

=item fh

The filehandle pointing to the uploaded file:

    my $fh = $upload->fh;
    while (<$fh>) {
	...
    }

=item size

The size of the file in bytes:

    my $size = $upload->size;

=item info

The additional header information for the uploaded file.
Returns a hash reference tied to the I<Apache::Table> class.
An optional I<key> argument can be passed to return the value of 
a given header rather than a hash reference.  Examples:

    my $info = $upload->info;
    while (my($key, $val) = each %$info) {
	...
    }

    my $val = $upload->info("Content-type");

=item type

Returns the I<Content-Type> for the given I<Apache::Upload> object:

    my $type = $upload->type;
    #same as
    my $type = $upload->info("Content-Type");

=item next

As an alternative to using the I<Apache::Request> I<upload> method in
an array context:

    for (my $upload = $apr->upload; $upload; $upload = $upload->next) {
	...
    }

    #functionally the same as:

    for my $upload ($apr->upload) {
	...
    }

=back

=head1 CREDITS

This interface is based on the original pure Perl version by Lincoln Stein.

=head1 AUTHOR

Doug MacEachern
