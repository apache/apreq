package Apache2::Cookie;
use APR::Request::Cookie;
use APR::Request::Apache2;
use APR::Request qw/encode decode/;

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
    $r = APR::Request::Apache2->new($r) unless $r->isa("APR::Request");
    $cookie->bind_handle($r);
    $cookie;
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
    my $self = shift;
    my @rv = split /&/, @_ ? shift : "$self";
    return wantarray ? map decode($_), @rv : decode($rv[0]);
}

sub value {
    return shift->thaw;
}

package Apache2::Cookie::Jar;
use APR::Request::Apache2;
push our @ISA, qw/APR::Request::Apache2/;
sub cookies { Apache2::Cookie->fetch(shift) }

1;
