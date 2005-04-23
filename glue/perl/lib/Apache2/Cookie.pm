package Apache2::Cookie;
use APR::Table;
use APR::Pool;
use APR::Request::Cookie;
use APR::Request::Apache2;
use APR::Request qw/encode decode/;
use Apache2::RequestRec;
use overload '""' => sub { shift->as_string() }, fallback => 1;

push our @ISA, "APR::Request::Cookie";

sub new {
    my ($class, $r, %attrs) = @_;
    my $name  = delete $attrs{name};
    my $value = delete $attrs{value};
    $name     = delete $attrs{-name}  unless defined $name;
    $value    = delete $attrs{-value} unless defined $value;
    return unless defined $name and defined $value;

    my $cookie = $class->make($r->pool, $name,
                              $class->freeze($value));

    while(my ($k, $v) = each %attrs) {
        $k =~ s/^-//;
        $cookie->$k($v);
    }
    return $cookie;
}


sub fetch {
    my $class = shift;
    my $req = shift;
    unless (defined $req) {
        my $usage = 'Usage: Apache2::Cookie->fetch($r): missing argument $r';
        $req = eval {Apache2->request} or die <<EOD;
$usage: attempt to fetch global Apache->request failed: $@.
EOD
    }
    $req = APR::Request::Apache2->new($req) unless $req->isa("APR::Request");
    my $jar = $req->jar or return;
    $jar->cookie_class(__PACKAGE__);
    return wantarray ? %$jar : $jar;
}


sub set_attr {
    my ($cookie, %attrs) = @_;
    while (my ($k, $v) = each %attrs) {
        $k =~ s/^-//;
        $cookie->$k($v);
    }
}

sub freeze {
    my ($class, $value) = @_;
    die 'Usage: Apache2::Cookie->freeze($value)' unless @_ == 2;

    if (not ref $value) {
        return encode($value);
    }
    elsif (UNIVERSAL::isa($value, "ARRAY")) {
        return join '&', map encode($_), @$value;
    }
    elsif (UNIVERSAL::isa($value, "HASH")) {
        return join '&', map encode($_), %$value;
    }

    die "Can't freeze reference: $value";
}

sub thaw {
    my $c = shift;
    my @rv = split /&/, @_ ? shift : $c->SUPER::value;
    return wantarray ? map decode($_), @rv : decode($rv[0]);
}

sub value {
    return shift->thaw;
}

sub bake {
    my ($c, $r) = @_;
    $r->err_headers_out->add("Set-Cookie", $c->as_string);
}

sub bake2 {
    my ($c, $r) = @_;
    die "Can't bake2 a Netscape cookie: $c" unless $c->version > 0;
    $r->err_headers_out->add("Set-Cookie2", $c->as_string);
}


package Apache2::Cookie::Jar;
use APR::Request::Apache2;
push our @ISA, qw/APR::Request::Apache2/;
sub cookies { Apache2::Cookie->fetch(shift) }
*Apache2::Cookie::Jar::status = *APR::Request::jar_status;

my %old_args = (
    value_class => "cookie_class",
);

sub new {
    my $class = shift;
    my $jar = $class->APR::Request::Apache2::new(shift);
    my %attrs = @_;
    while (my ($k, $v) = each %attrs) {
        $k =~ s/^-//;
        my $method = $old_args{lc($k)} || lc $k;
        $jar->$method($v);
    }
    return $jar;
}

1;
