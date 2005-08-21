package APR::Request::Cookie;
use APR::Request;

sub new {
    my ($class, $pool, %attrs) = @_;
    my $name  = delete $attrs{name};
    my $value = delete $attrs{value};
    $name     = delete $attrs{-name}  unless defined $name;
    $value    = delete $attrs{-value} unless defined $value;
    return unless defined $name and defined $value;

    my $cookie = $class->make($pool, $name, $class->freeze($value));
    while(my ($k, $v) = each %attrs) {
        $k =~ s/^-//;
        $cookie->$k($v);
    }
    return $cookie;
}

sub freeze { return $_[1] }
sub thaw {
    my $obj = shift;
    return shift if @_;
    return "$obj";
}

package APR::Request::Cookie::Table;

sub EXISTS {
    my ($t, $key) = @_;
    return defined $t->FETCH($key);
}