{
    package APR::Request::Error;
    require APR::Error;
    push our @ISA, qw/APR::Error APR::Request/;
}

sub import {
    my $class = shift;
    return unless @_;
    my $pkg = caller;
    no strict 'refs';
    for (@_) {
        *{"$pkg\::$_"} = *{"$class\::$_"};
    }
}
