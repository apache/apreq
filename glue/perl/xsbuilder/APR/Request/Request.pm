sub import {
    my $class = shift;
    return unless @_;
    my $pkg = caller;
    no strict 'refs';

    for (@_) {
        *{"$pkg\::$_"} = $class->can($_)
            or die "Can't find method $_ in class $class";
    }
}
