{
    package APR::Request::Error;
    require APR::Error;
    push our @ISA, qw/APR::Error APR::Request/;
}
