package Apache::Request;

use strict;
use mod_perl 1.17_01;
use Apache::Table ();

{
    no strict;
    $VERSION = '0.3104';
    @ISA = qw(Apache);
    __PACKAGE__->mod_perl::boot($VERSION);
}

#just prototype methods here, moving to xs later

sub instance {
    my $class = shift;
    my $r = shift;
    if (my $apreq = $r->pnotes('apreq')) {
        return $apreq;
    }
    my $new_req = $class->new($r, @_);
    $r->pnotes('apreq', $new_req);
    return $new_req;
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
I<multipart/form-data>. See the libapreq(3) manpage for more details.

=head1 Apache::Request METHODS

=over 4

=head2 new

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

=item TEMP_DIR

Sets the directory where upload files are spooled.  On a *nix-like
that supports link(2), the TEMP_DIR should be placed on the same
file system as the final destination file:

 my $apr = Apache::Request->new($r, TEMP_DIR => "/home/httpd/tmp");
 my $upload = $apr->upload('file');
 $upload->link("/home/user/myfile") || warn "link failed: $!";

=back

=head2 instance

The instance() class method allows Apache::Request to be a singleton.
This means that whenever you call Apache::Request->instance() within a
single request you always get the same Apache::Request object back.
This solves the problem with creating the Apache::Request object twice
within the same request - the symptoms being that the second
Apache::Request object will not contain the form parameters because
they have already been read and parsed.

  my $apr = Apache::Request->instance($r, DISABLE_UPLOADS => 1);

Note that C<instance()> call will take the same parameters as the above
call to C<new()>, however the parameters will only have an effect the
first time C<instance()> is called within a single request. Extra
parameters will be ignored on subsequent calls to C<instance()> within
the same request.

Subrequests receive a new Apache::Request object when they call
instance() - the parent request's Apache::Request object is not copied
into the subrequest.

Also note that it is unwise to use the C<parse()> method when using
C<instance()> because you may end up trying to call it twice, and
detecting errors where there are none.

=head2 parse

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

=head2 param

Get or set request parameters:

    my $value = $apr->param('foo');
    my @values = $apr->param('foo');
    my @params = $apr->param;
    $apr->param('foo' => [qw(one two three)]);

=head2 upload

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

=head2 name

The name of the filefield parameter:

    my $name = $upload->name;

=head2 filename

The filename of the uploaded file:

    my $filename = $upload->filename;

=head2 fh

The filehandle pointing to the uploaded file:

    my $fh = $upload->fh;
    while (<$fh>) {
	...
    }

=head2 size

The size of the file in bytes:

    my $size = $upload->size;

=head2 info

The additional header information for the uploaded file.
Returns a hash reference tied to the I<Apache::Table> class.
An optional I<key> argument can be passed to return the value of 
a given header rather than a hash reference.  Examples:

    my $info = $upload->info;
    while (my($key, $val) = each %$info) {
	...
    }

    my $val = $upload->info("Content-type");

=head2 type

Returns the I<Content-Type> for the given I<Apache::Upload> object:

    my $type = $upload->type;
    #same as
    my $type = $upload->info("Content-Type");

=head2 next

As an alternative to using the I<Apache::Request> I<upload> method in
an array context:

    for (my $upload = $apr->upload; $upload; $upload = $upload->next) {
	...
    }

    #functionally the same as:

    for my $upload ($apr->upload) {
	...
    }

=head2 tempname

Provides the name of the spool file.

=head2 link

To avoid recopying the spool file on a Unix-like system,
I<link> will create a hard link to it:

  my $upload = $apr->upload('file');
  $upload->link("/path/to/newfile") or
      die sprintf "link from '%s' failed: $!", $upload->tempname;

Typically the new name must lie on the same file system as the
spool file. Check your system's link(2) manpage for details.

=back

=head1 SEE ALSO

libapreq(3)

=head1 CREDITS

This interface is based on the original pure Perl version by Lincoln Stein.

=head1 AUTHOR

Doug MacEachern
