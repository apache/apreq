package Apache2::Request;
use APR::Request::Param;
use APR::Request::Apache2 qw/args/; # XXX the args() override here is a bug.
use Apache2::RequestRec;
push our @ISA, qw/Apache2::RequestRec APR::Request::Apache2/;

my %old_limits = (
    post_max => "read_limit",
    max_body => "read_limit",
);

sub new {
    my $class = shift;
    my $req = $class->APR::Request::Apache2::new(shift);
    my %attrs = @_;

    while (my ($k, $v) = each %attrs) {
        $k =~ s/^-//;
        my $method = $old_limits{lc($k)} || lc $k;
        $req->$method($v);
    }
    return $req;
}

sub hook_data {die "hook_data not implemented yet"}
sub upload_hook {die "upload_hook not implemented yet"}
sub disable_uploads {
    my ($req, $pool) = @_;
    $pool ||= $req->pool;
    $req->APR::Request::disable_uploads($pool);
}

1;
