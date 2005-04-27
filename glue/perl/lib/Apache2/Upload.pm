package Apache2::Upload;
use Apache2::Request;
push our @ISA, qw/APR::Request::Param/;
our $VERSION = "2.05-dev";
no strict 'refs';
for (qw/slurp type size link tempname fh io filename/) {
    *{$_} = *{"APR::Request::Param::upload_$_"}{CODE};
}
sub Apache2::Request::upload {
    my $req = shift;
    my $body = $req->body or return;
    $body->param_class(__PACKAGE__);
    if (@_) {
        my @uploads = grep $_->upload, $body->get(@_);
        return wantarray ? @uploads : $uploads[0];
    }

    return map { $_->upload ? $_->name : () } values %$body
        if wantarray;

   return $body->uploads($req->pool);

}

*bb = *APR::Request::Param::upload;

1;

__END__

=head1 NAME

Apache2::Upload - Methods for dealing with file uploads.

=for testing
    use APR::Pool;
    use Apache2::Upload;
    $r = APR::Pool->new;
    $req = Apache2::Request->new($r);
    $u = Apache2::Upload->new($r, name => "foo", file => __FILE__);
    $req->body_status(0);
    $req->parse;
    $req->body->add(foo => "bar"); # dummy param with same name as upload
    $req->body->add($u);
    open(my $fh, __FILE__) or die $!;
    binmode $fh;
    {
      local $/;
      $data = <$fh>;
    }
    close $fh;
    ok length $data == -s __FILE__;
    $data =~ s{\r}{}g;



=head1 SYNOPSIS


=for example begin

    use Apache2::Upload;

    $req = Apache2::Request->new($r);
    $upload = $req->upload("foo");
    $size = $upload->size;

    # three methods to get at the upload's contents ... slurp, fh, io

    $upload->slurp($slurp_data);

    read $upload->fh, $fh_data, $size;
    ok $slurp_data eq $fh_data;

    my $io = $upload->io;
    print while <$io>;

=for example end

=for example_testing
    ok $upload->bb->length == $size;
    $uploads = $req->upload();
    is (scalar keys %{$uploads}, 1, "found upload");
    is $_STDOUT_, $data;
    is $fh_data, $data;
    is $slurp_data, $data;





=head1 DESCRIPTION

Apache2::Upload is a new module based on the original package included
in Apache2::Request 1.X.  Users requiring the upload API must now 
C<use Apache2::Upload>, which adds the C<upload> method to Apache2::Request.
Apache2::Upload is largely backwards-compatible with the original 1.X API.
See the L<PORTING from 1.X> section below for a list of known issues.

This manpage documents the Apache2::Upload package.




=head1 Apache2::Upload




=head2 name

    $upload->name()

The name of the HTML form element which generated the upload.

=for testing
    is $u->name, "foo";




=head2 filename

    $upload->filename()

The (client-side) filename as submitted in the HTML form.  Note: 
some agents will submit the file's full pathname, while others 
may submit just the basename.

=for testing
    is $u->filename, __FILE__;




=head2 fh

    $upload->fh()

Creates filehandle reference to the upload's spooled tempfile,
which contains the full contents of the upload.




=head2 io

    $upload->io()

Creates a tied IO handle.  This method is a more efficient version 
of C<fh>, but with C<io> the handle ref returned is not seekable.  
It is tied to an APR::Request::Brigade object, so you may use the 
brigade API on the tied object if you want to manipulate the IO stream 
(beyond simply reading from it).

The returned reference is actually an object which has C<read> and 
C<readline> methods available.  However these methods are just 
syntactic sugar for the underlying C<READ> and C<READLINE> methods from 
APR::Request::Brigade.  

=for example begin

    $io = $upload->io;
    print while $io->read($_); # equivalent to: tied(*$io)->READ($_)

=for example end

=for example_testing
    is $_STDOUT_, $data;
    $io = $upload->io;
    $io->read($h{io}, $upload->size);
    is $h{io}, $data, "autovivifying read";

See L<READ|read> and L<READLINE|readline> below for additional notes 
on their usage.



=head2 bb

    $upload->bb()
    $upload->bb($set)

Get/set the APR::Brigade which represents the upload's contents.




=head2 size

    $upload->size()

Returns the size of the upload in bytes.

=for testing
    is $u->size, -s __FILE__;




=head2 info

    $upload->info()
    $upload->info($set)

Get/set the additional header information table for the 
uploaded file.
Returns a hash reference tied to the I<APR::Table> class.
An optional C<$table> argument can be passed to reassign
the upload's internal (apr_table_t) info table to the one
C<$table> represents.

=for example begin

    my $info = $upload->info;
    while (my($hdr_name, $hdr_value) = each %$info) {
	# ...
    }

    # fetch upload's Content-Type header
    my $type = $upload->info->{"Content-type"};

=for example end

=for example_testing
    is $type, "application/octet-stream";




=head2 type

    $upload->type()

Returns the MIME type of the given I<Apache2::Upload> object.

=for example begin

    my $type = $upload->type;

    #same as
    my $content_type = $upload->info->{"Content-Type"};
    $content_type =~ s/;.*$//ms;

=for example end

=for example_testing
    is $type, $content_type;




=head2 link

    $upload->link()

To avoid recopying the upload's internal tempfile brigade on a 
*nix-like system, I<link> will create a hard link to it:

=for example begin

  my $upload = $req->upload('foo');
  $upload->link("/path/to/newfile") or
      die sprintf "link from '%s' failed: $!", $upload->tempname;

=for example end

Typically the new name must lie on the same device and partition 
as the brigade's tempfile.  If this or any other reason prevents
the OS from linking the files, C<link()> will instead 
copy the temporary file to the specified location.




=head2 slurp

    $upload->slurp($contents)


Reads the full contents of a file upload into the scalar argument.
The return value is the length of the file.

=for example begin

    my $size = $upload->slurp($contents);

=for example end

=for example_testing
    is $size, length $contents;
    $upload->slurp($h{slurp});
    is $h{slurp}, $contents, "autovivifying slurp";
    $contents =~ s{\r}{}g;
    is $contents, $data;




=head2 tempname

    $upload->tempname()

Provides the name of the spool file.

=for example begin

    my $tempname = $upload->tempname;

=for example end

=for example_testing
    like $tempname, qr/apreq.{6}$/;




=head1 APR::Request::Brigade


This class is derived from APR::Brigade, providing additional
methods for TIEHANDLE, READ and READLINE.  To be memory efficient,
these methods delete buckets from the brigade as soon as their 
data is actually read, so you cannot C<seek> on handles tied to
this class.  Such handles have semantics similar to that of a 
read-only socket.




=head2 TIEHANDLE

    APR::Request::Brigade->TIEHANDLE($bb)

Creates a copy of the bucket brigade represented by $bb, and
blesses that copy into the APR::Request::Brigade class.  This
provides syntactic sugar for using perl's builtin C<read>, C<readline>,
and C<< <> >> operations on handles tied to this package:

    use Symbol;
    $fh = gensym;
    tie *$fh, "APR::Request:Brigade", $bb;
    print while <$fh>;




=head2 READ

    $bb->READ($contents)
    $bb->READ($contents, $length)
    $bb->READ($contents, $length, $offset)

Reads data from the brigade $bb into $contents.  When omitted
$length defaults to C<-1>, which reads the first bucket into $contents.  
A positive $length will read in $length bytes, or the remainder of the 
brigade, whichever is greater. $offset represents the index in $context 
to read the new data.




=head2 READLINE

    $bb->READLINE()

Returns the first line of data from the bride. Lines are terminated by
linefeeds (the '\012' character), but we may eventually use C<$/> instead.




=head1 PORTING from 1.X

=over 4

=item * C<< $upload->next() >> is no longer available;  please use the 
C<< APR::Request::Param::Table >> API when iterating over upload entries.

=item * C<info($header_name)> is replaced by C<info($set)>.

=item * C<type()> returns only the MIME-type portion of the Content-Type header.

=back




=head1 SEE ALSO

L<APR::Request::Param::Table>, L<APR::Request::Error>, L<Apache2::Request>,
APR::Table(3)




=head1 COPYRIGHT

  Copyright 2003-2005  The Apache Software Foundation

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
