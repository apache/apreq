#!/usr/bin/perl
# requires successful ./configure && make
#
# expected usage: cd glue/perl; ../../build/xsbuilder.pl run run
#

use strict;
use warnings FATAL => 'all';
my $version = "2.XX-dev"; # DUMMY VALUE
use File::Basename;
use Apache2;
use Apache::Build;
use constant WIN32 => Apache::Build::WIN32;
use Cwd;
my $cwd = WIN32 ?
    Win32::GetLongPathName(cwd) : cwd;
$cwd =~ m{^(.+)/glue/perl$} or die "Can't find base directory";
my $base_dir = $1;
my $src_dir = "$base_dir/src";
my $xs_dir = "$base_dir/glue/perl/xsbuilder";
sub slurp($$)
{
    open my $file, $_[1] or die "Can't open $_[1]: $!";
    read $file, $_[0], -s $file;
}

my ($apache_includes, $apr_libs, $apreq_libname);

if (WIN32) {
    # XXX May need fixing, Randy!
    slurp my $config => "$base_dir/configure.ac";
    $config =~ /^AC_INIT[^,]+,\s*([^,\s]+)/m or 
        die "Can't find version string";
    $version = $1;
    slurp my $make => "$base_dir/Makefile";
    $make =~ /^APACHE=(\S+)/m or
        die "Cannot find top-level Apache directory";
    my $apache_dir = $1;
    ($apache_includes = "-I$apache_dir" . '/include') =~ s!\\!/!g;
    ($apr_libs = "-L$apache_dir" . '/lib') =~ s!\\!/!g;
    $make =~ /^APR_LIB=(\S+)/m or
        die "Cannot find apr lib";
    $apr_libs .= ' -l' . basename($1, '.lib');
    $make =~ /^APU_LIB=(\S+)/m or
        die "Cannot find aprutil lib";
    $apr_libs .= ' -l' . basename($1, '.lib');
    $apreq_libname = 'apreq2';
}
else {
    slurp my $config => "$base_dir/config.status";
    $config =~ /^s,\@APACHE2_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes = $1;
    $config =~ /^s,\@APR_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes .= " $1";
    $config =~ /^s,\@APU_INCLUDES\@,([^,]+)/m or
        die "Can't find apache include directory";
    $apache_includes .= " $1";

    $config =~ m/^s,\@APR_LDFLAGS\@,([^,]+)/m or
        die "Can't find apr ldflags";
    $apr_libs = $1;
    $config =~ m/^s,\@APU_LDFLAGS\@,([^,]+)/m or
        die "Can't find apu ldflags";
    $apr_libs .= " $1";

    # need -laprutil befpre -lapr
    $config =~ m/^s,\@APU_LDLIBS\@,([^,]+)/m or
        die "Can't find apu libraries";
    $apr_libs = $1;
    $config =~ m/^s,\@APR_LDLIBS\@,([^,]+)/m or
        die "Can't find apr libraries";
    $apr_libs .= " $1";

    $config =~ m/^s,\@APREQ_LIBNAME\@,([^,]+)/m or
        die "Can't find apreq libname";
    $apreq_libname = $1;

    $config =~ m/^s,\@PACKAGE_VERSION\@,([^,]+)/m or
        die "Can't find package version";
    $version = $1;

}

my $apreq_libs = WIN32 ?
    qq{-L$base_dir/win32/libs -llib$apreq_libname } :
    qq{-L$src_dir/.libs -l$apreq_libname};

my $mp2_typemaps = Apache::Build->new->typemaps;
read DATA, my $grammar, -s DATA;

my %c_macro_cache = (
                     XS => sub {s/XS\s*\(([^)]+)\)/void $1()/g},

);
sub c_macro
{
    return $c_macro_cache{"@_"} if exists $c_macro_cache{"@_"};

    my ($name, $header) = @_;
    my $src;
    if (defined $header) {
        slurp local $_ => "$src_dir/$header";
        /^#define $name\s*\(([^)]+)\)\s+(.+?[^\\])$/ms
            or die "Can't find definition for '$name': $_";
        my $def = $2;
        my @args = split /\s*,\s*/, $1;
        for (1..@args) {
            $def =~ s/\b$args[$_-1]\b/ \$$_ /g;
        }
        my $args = join ',' => ('([^,)]+)') x @args;
        $src = "sub { /^#define $name.+?[^\\\\]\$/gms +
                      s{$name\\s*\\($args\\)}{$def}g}";
    }
    else {
        $src = "sub { /^#define $name.+?[^\\\\]\$/gms +
                      s{$name\\s*\\(([^)]+)\\)}{\$1}g}";
    }
    return $c_macro_cache{"@_"} = eval $src;
}


package My::ParseSource;
use constant WIN32 => ($^O =~ /Win32/i);
my @dirs = ("$base_dir/src", "$base_dir/glue/perl/xsbuilder");
use base qw/ExtUtils::XSBuilder::ParseSource/;
__PACKAGE__->$_ for shift || ();
system("touch $base_dir/glue/perl/xsbuilder") == 0
    or die "touch $base_dir/glue/perl/xsbuilder failed: $!"
    unless WIN32;

sub package {'Apache::libapreq2'}
sub unwanted_includes {[qw/apreq_tables.h apreq_config.h/]}
# ParseSource.pm v 0.23 bug: line 214 should read
# my @dirs = @{$self->include_dirs};
# for now, we override it here just to work around the bug

sub find_includes {
    my $self = shift;
    return $self->{includes} if $self->{includes};
    require File::Find;
    my(@dirs) = @{$self->include_dirs};
    unless (-d $dirs[0]) {
        die "could not find include directory";
    }
    # print "Will search @dirs for include files...\n" if ($verbose) ;
    my @includes;
    my $unwanted = join '|', @{$self -> unwanted_includes} ;

    for my $dir (@dirs) {
        File::Find::finddepth({
                               wanted => sub {
                                   return unless /\.h$/;
                                   return if ($unwanted && (/^($unwanted)/o));
                                   my $dir = $File::Find::dir;
                                   push @includes, "$dir/$_";
                               },
                               follow => $^O ne 'MSWin32',
                              }, $dir);
    }
    return $self->{includes} = $self -> sort_includes (\@includes) ;
}

sub include_dirs {\@dirs}

sub preprocess
{
    # need to macro-expand APREQ_DECLARE et.al. so P::RD can DTRT with
    # ExtUtils::XSBuilder::C::grammar

    for ($_[1]) {
        ::c_macro("APREQ_DECLARE", "apreq.h")->();
        ::c_macro("APREQ_DECLARE_HOOK", "apreq_params.h")->();
        ::c_macro("APREQ_DECLARE_PARSER", "apreq_params.h")->();
        ::c_macro("APR_DECLARE")->();
        ::c_macro("XS")-> ();
    }
}

sub parse {
    my $self = shift;
    our $verbose = $ExtUtils::XSBuilder::ParseSource::verbose;

    $self -> find_includes ;
    my $c = $self -> {c} = {} ;
    print "Initialize parser\n" if ($verbose) ;

    $::RD_HINT++;

    my $parser = $self -> {parser} = Parse::RecDescent->new($grammar);

    $parser -> {data} = $c ;
    $parser -> {srcobj} = $self ;

    $self -> extent_parser ($parser) ;

    foreach my $inc (@{$self->{includes}}) {
        print "scan $inc ...\n" if ($verbose) ;
        $self->scan ($inc) ;
    }

}


package My::WrapXS;
pop @dirs; # drop mod_h directories- WrapXS takes care of those
use base qw/ExtUtils::XSBuilder::WrapXS/;
our $VERSION = $version;
__PACKAGE__ -> $_ for @ARGV;

sub parsesource_objects {[My::ParseSource->new]}
sub new_typemap {My::TypeMap->new(shift)}
sub h_filename_prefix {'apreq_xs_'}
sub my_xs_prefix {'apreq_xs_'}
sub xs_include_dir { $xs_dir }

sub mod_pod {
    my($self, $module, $complete) = @_;
    my $dirname = $self->class_dirname($module);
    my @parts = split '::', $module;
    my $mod_pod = "$dirname/$parts[-1]_pod";
    for ($self -> xs_incsrc_dir, @{ $self->{glue_dirs} }) {
        my $file = "$_/$mod_pod";
        $mod_pod = $file if $complete;
        print "mod_pod $mod_pod $file $complete\n" ;
        return $mod_pod if -e $file;
    }
    undef;
}

sub write_docs {
    my ($self, $module, $functions) = @_;
    my $podfile = $self->mod_pod($module, 1) or return;
    my $fh = $self->open_class_file($module, '.pod');
    open my $pod, "<", $podfile or die $!;
    while (<$pod>) {
        print $fh $_;
    }
}
sub pm_text {
    my($self, $module, $isa, $code) = @_;

    my $text = <<"EOF";
$self->{noedit_warning_hash}

package $module;
require DynaLoader ;

use strict;
use warnings FATAL => 'all';

use vars qw{\$VERSION \@ISA} ;
$isa
push \@ISA, 'DynaLoader' ;
\$VERSION = '$version';
bootstrap $module \$VERSION ;
EOF

    $text .= <<'EOF';

if ($ENV{MOD_PERL}) {
    require mod_perl;
    my $env = __PACKAGE__->env || '';
    if ($mod_perl::VERSION > 1.99) {
        die __PACKAGE__ . ": httpd must load mod_apreq.so first"
            if $env ne "Apache::RequestRec";
    }
    else {
        die "Unsupported mod_perl version number: $modperl::VERSION";
    }
}
EOF

    $text .= <<"EOF";

$code

1;
__END__
EOF

        return $text;
}
sub makefilepl_text {
    my($self, $class, $deps,$typemap) = @_;

    my @parts = split (/::/, $class) ;
    my $mmargspath = '../' x @parts ;
    $mmargspath .= 'mmargs.pl' ;

    my $txt = qq{
$self->{noedit_warning_hash}
use Apache2;
use ModPerl::MM;

local \$MMARGS ;

if (-f '$mmargspath')
    {
    do '$mmargspath' ;
    die \$\@ if (\$\@) ;
    }

\$MMARGS ||= {} ;


ModPerl::MM::WriteMakefile(
    'NAME'      => '$class',
    'VERSION'   => '$version',
    'TYPEMAPS'  => [qw(@$mp2_typemaps $typemap)],
    'INC'       => "-I$base_dir/glue/perl/xs -I$src_dir -I$xs_dir $apache_includes",
    'LIBS'      => "$apreq_libs $apr_libs",
} ;
$txt .= "'depend'  => $deps,\n" if ($deps) ;
$txt .= qq{    
    \%\$MMARGS,
);

} ;

}

# another bug in WrapXS.pm - 
# must insert a space before typemap definition

sub write_typemap
{
    my $self = shift;
    my $typemap = $self->typemap;
    my $map = $typemap->get;
    my %seen;

    my $fh = $self->open_class_file('', 'typemap');
    print $fh "$self->{noedit_warning_hash}\n";

    while (my($type, $t) = each %$map) {
        my $class = $t -> {class} ;
        $class ||= $type;
        next if $seen{$type}++ || $typemap->special($class);

        my $typemap = $t -> {typemapid} ;
        if ($class =~ /::/) {
            next if $seen{$class}++ ;
            $class =~ s/::$// ;
            print $fh "$class\t$typemap\n";
        }
        else {
            print $fh "$type\t$typemap\n";
        }
    }

    my $typemap_code = $typemap -> typemap_code ;

    foreach my $dir ('INPUT', 'OUTPUT') {
        print $fh "\n$dir\n" ;
        while (my($type, $code) = each %{$typemap_code}) {
            print $fh "$type\n\t$code->{$dir}\n\n" if ($code->{$dir}) ;
        }
    }

    close $fh;
}


package My::TypeMap;
use base 'ExtUtils::XSBuilder::TypeMap';

sub null_type {
    my($self, $type) = @_;
    my $t = $self->get->{$type};
    my $class = $t -> {class} ;

    if ($class =~ /APREQ_COOKIE_VERSION/) {
        return 'APREQ_COOKIE_VERSION_DEFAULT';
    }
    else {
        return $self->SUPER::null_type($type);
    }
}

# XXX this needs serious work
sub typemap_code
{
    {
        T_APREQ_COOKIE  => {
                            INPUT  => '$var = apreq_xs_sv2(cookie,$arg)',
                            perl2c => 'apreq_xs_sv2(cookie,sv)',
                            OUTPUT => '$arg = apreq_xs_2sv($var,"\${ntype}\");',
                            c2perl => 'apreq_xs_2sv(ptr,\"$class\")',
                           },
        T_APREQ_PARAM   => {
                            INPUT  => '$var = apreq_xs_sv2param($arg)',
                            perl2c => 'apreq_xs_sv2param(sv)',
                            OUTPUT => '$arg = apreq_xs_param2sv($var);',
                            c2perl => 'apreq_xs_param2sv(ptr)',
                           },
        T_APREQ_REQUEST => {
                            INPUT  => '$var = apreq_xs_sv2(request,$arg)',
                            perl2c => 'apreq_xs_sv2(request,sv)',
                            OUTPUT => '$arg = apreq_xs_2sv($var,\"${ntype}\");',
                            c2perl => 'apreq_xs_2sv(ptr,\"$class\")',
                           },
        T_APREQ_JAR     => {
                            INPUT  => '$var = apreq_xs_sv2(jar,$arg)',
                            perl2c => 'apreq_xs_sv2(jar,sv)',
                            OUTPUT => '$arg = apreq_xs_2sv($var,\"${ntype}\");',
                            c2perl => 'apreq_xs_2sv(ptr,\"$class\")',
                           },
 T_APREQ_COOKIE_VERSION => {
                            INPUT  => '$var = ((apreq_cookie_version_t)SvTRUE($arg))',
                            OUTPUT => '$arg = boolSV((bool)$var);',
                           },
        T_HASHOBJ       => {
                            INPUT => <<'EOT', # '$var = modperl_hash_tied_object(aTHX_ \"${ntype}\", $arg)'
    if (sv_derived_from($arg, \"${ntype}\")) {
        if (SVt_PVHV == SvTYPE(SvRV($arg))) {
            SV *hv = SvRV($arg);
            MAGIC *mg;
            if (SvMAGICAL(hv)) {
                if ((mg = mg_find(hv, PERL_MAGIC_tied))) {
                    $var = (void *)MgObjIV(mg);
                }
                else {
                    Perl_warn(aTHX_ \"Not a tied hash: (magic=%c)\", mg);
                    $var = NULL;
                }
            }
            else {
                Perl_warn(aTHX_ \"SV is not tied\");
                $var = NULL;
            }
        }
        else {
            $var = (void *)SvObjIV($arg);
        }
    }
    else {
        Perl_croak(aTHX_
                   \"argument is not a blessed reference \"
                   \"(expecting an %s derived object)\", \"${ntype}\");
    }
EOT

                            OUTPUT => <<'EOT', # '$arg = modperl_hash_tie(aTHX_ \"${ntype}\", $arg, $var);'
  {
    SV *hv = (SV*)newHV();
    SV *rsv = $arg;
    sv_setref_pv(rsv, \"${ntype}\", $var);
    sv_magic(hv, rsv, PERL_MAGIC_tied, Nullch, 0);
    $arg = SvREFCNT_inc(sv_bless(sv_2mortal(newRV_noinc(hv)),
                                 gv_stashpv(\"${ntype}\", TRUE)));
  }
EOT

                           },
    }
}

# force DATA into main package
package main;
1;

__DATA__
{ 
use ExtUtils::XSBuilder::C::grammar ; # import cdef_xxx functions 
}

code:	comment_part(s) {1}

comment_part:
    comment(s?) part
        { 
        #print "comment: ", Data::Dumper::Dumper(\@item) ;
        $item[2] -> {comment} = "@{$item[1]}" if (ref $item[1] && @{$item[1]} && ref $item[2]) ;
        1 ;
        }
    | comment

part:   
    prepart 
    | stdpart
        {
        if ($thisparser -> {my_neednewline}) 
            {
            print "\n" ;
            $thisparser -> {my_neednewline} = 0 ;
            }
        $return = $item[1] ;
        }

# prepart can be used to extent the parser (for default it always fails)

prepart:  '?' 
        {0}

           
stdpart:   
    define
        {
        $return = cdef_define ($thisparser, $item[1][0], $item[1][1]) ;
        }
    | struct
        {
        $return = cdef_struct ($thisparser, @{$item[1]}) ;
        }
    | enum
        {
        $return = cdef_enum ($thisparser, $item[1][1]) ;
        }
    | function_declaration
        {
        $return = cdef_function_declaration ($thisparser, @{$item[1]}) ;
        }
    | struct_typedef
        {
        my ($type,$alias) = @{$item[1]}[0,1];
        $return = cdef_struct ($thisparser, undef, $type, undef, $alias) ;
        }
    | comment
    | anything_else

comment:
    m{\s* // \s* ([^\n]*) \s*? \n }x
        { $1 }
    | m{\s* /\* \s* ([^*]+|\*(?!/))* \s*? \*/  ([ \t]*)? }x
        { $item[1] =~ m#/\*\s*?(.*?)\s*?\*/#s ; $1 }

semi_linecomment:
    m{;\s*\n}x
        {
        $return = [] ;
        1 ;
        }
    | ';' comment(s?)
        {
        $item[2]
        }

function_definition:
    rtype IDENTIFIER '(' <leftop: arg ',' arg>(s?) ')' '{'
        {[@item[2,1], $item[4]]}

pTHX:
    'pTHX_'

function_declaration:
    type_identifier '(' pTHX(?) <leftop: arg_decl ',' arg_decl>(s?) ')' function_declaration_attr ( ';' | '{' )
        {
        #print Data::Dumper::Dumper (\@item) ;
            [
            $item[1][1], 
            $item[1][0], 
            @{$item[3]}?[['pTHX', 'aTHX' ], @{$item[4]}]:$item[4] 
            ]
        }

define:
    '#define' IDENTIFIER /.*?\n/
        {
        $item[3] =~ m{(?:/\*\s*(.*?)\s*\*/|//\s*(.*?)\s*$)} ; [$item[2], $1] 
        }

ignore_cpp:
    '#' /.*?\n/

struct: 
    'struct' IDENTIFIER '{' field(s) '}' ';'
        {
        # [perlname, cname, fields]
        [$item[2], "@item[1,2]", $item[4]]
        }
    | 'typedef' 'struct' '{' field(s) '}' IDENTIFIER ';'
        {
        # [perlname, cname, fields]
        [$item[6], undef, $item[4], $item[6]]
        }
    | 'typedef' 'struct' IDENTIFIER '{' field(s) '}' IDENTIFIER ';'
        {
        # [perlname, cname, fields, alias]
        [$item[3], "@item[2,3]", $item[5], $item[7]]
        }

struct_typedef: 
    'typedef' 'struct' IDENTIFIER IDENTIFIER ';'
        {
	["@item[2,3]", $item[4]]
	}

enum: 
    'enum' IDENTIFIER '{' enumfield(s) '}' ';'
        {
        [$item[2], $item[4]]
        }
    | 'typedef' 'enum' '{' enumfield(s) '}' IDENTIFIER ';'
        {
        [undef, $item[4], $item[6]]
        }
    | 'typedef' 'enum' IDENTIFIER '{' enumfield(s) '}' IDENTIFIER ';'
        {
        [$item[3], $item[5], $item[7]]
        }

field: 
    comment 
    | define
	{
        $return = cdef_define ($thisparser, $item[1][0], $item[1][1]) ;
	}
    | valuefield 
    | callbackfield
    | ignore_cpp

valuefield: 
    type_identifier comment(s?) semi_linecomment
        {
        $thisparser -> {my_neednewline} = 1 ;
        print "  valuefield: $item[1][0] : $item[1][1]\n" ;
	[$item[1][0], $item[1][1], [$item[2]?@{$item[2]}:() , $item[3]?@{$item[3]}:()] ]
        }


callbackfield: 
    rtype '(' '*' IDENTIFIER ')' '(' <leftop: arg_decl ',' arg_decl>(s?) ')' comment(s?) semi_linecomment
        {
        my $type = "$item[1](*)(" . join(',', map { "$_->[0] $_->[1]" } @{$item[7]}) . ')' ;
        my $dummy = 'arg0' ;
        my @args ;
        for (@{$item[7]})
            {
            if (ref $_) 
                {
                push @args, { 
                    'type' => $_->[0], 
                    'name' => $_->[1], 
                    } if ($_->[0] ne 'void') ; 
                }
            }
        my $s = { 'name' => $type, 'return_type' => $item[1], args => \@args } ;
        push @{$thisparser->{data}{callbacks}}, $s  if ($thisparser->{srcobj}->handle_callback($s)) ;

        $thisparser -> {my_neednewline} = 1 ;
        print "  callbackfield: $type : $item[4]\n" ;
        [$type, $item[4], [$item[9]?@{$item[9]}:() , $item[10]?@{$item[10]}:()]] ;
        }


enumfield: 
    comment
    | IDENTIFIER  comment(s?) /,?/ comment(s?)
        {
        [$item[1], [$item[2]?@{$item[2]}:() , $item[4]?@{$item[4]}:()] ] ;
        }

rtype:  
    modmodifier(s) TYPE star(s?)
        {
        my @modifier = @{$item[1]} ;
        shift @modifier if ($modifier[0] eq 'extern' || $modifier[0] eq 'static') ;

        $return = join ' ',@modifier, $item[2] ;
        $return .= join '',' ',@{$item[3]} if @{$item[3]};
        1 ;
	}
    | TYPE(s) star(s?)
        {
        $return = join (' ', @{$item[1]}) ;
        $return .= join '',' ',@{$item[2]} if @{$item[2]};
	#print "rtype $return \n" ;
        1 ;
        }
    modifier(s)  star(s?)
        {
        join ' ',@{$item[1]}, @{$item[2]} ;
	}

arg:
    type_identifier 
        {[$item[1][0],$item[1][1]]}
    | '...'
        {['...']}

arg_decl: 
    rtype '(' '*' IDENTIFIER ')' '(' <leftop: arg_decl ',' arg_decl>(s?) ')'
        {
        my $type = "$item[1](*)(" . join(',', map { "$_->[0] $_->[1]" } @{$item[7]}) . ')' ;
        my $dummy = 'arg0' ;
        my @args ;
        for (@{$item[7]})
            {
            if (ref $_) 
                {
                push @args, { 
                    'type' => $_->[0], 
                    'name' => $_->[1], 
                    } if ($_->[0] ne 'void') ; 
                }
            }
        my $s = { 'name' => $type, 'return_type' => $item[1], args => \@args } ;
        push @{$thisparser->{data}{callbacks}}, $s  if ($thisparser->{srcobj}->handle_callback($s)) ;

        [$type, $item[4], [$item[9]?@{$item[9]}:() , $item[11]?@{$item[11]}:()]] ;
        }
    | 'pTHX'
	{
	['pTHX', 'aTHX' ]
	}
    | type_identifier
	{
	[$item[1][0], $item[1][1] ]
	}
    | '...'
        {['...']}

function_declaration_attr:

type_identifier:
    type_varname 
        { 
        my $r ;
	my @type = @{$item[1]} ;
	#print "type = @type\n" ;
	my $name = pop @type ;
	if (@type && ($name !~ /\*/)) 
	    {
            $r = [join (' ', @type), $name] 
	    }
	else
	    {
	    $r = [join (' ', @{$item[1]})] ;
	    }	            
	#print "r = @$r\n" ;
        $r ;
        }
 
type_varname:   
    attribute(s?) TYPE(s) star(s) varname(?)
        {
	[@{$item[1]}, @{$item[2]}, @{$item[3]}, @{$item[4]}] ;	
	}
    | attribute(s?) varname(s)
        {
	$item[2] ;	
	}


varname:
    ##IDENTIFIER '[' IDENTIFIER ']'
    IDENTIFIER '[' /[^]]+/ ']'
	{
	"$item[1]\[$item[3]\]" ;
	}
    | IDENTIFIER ':' IDENTIFIER
	{
	$item[1]
	}
    | IDENTIFIER
	{
	$item[1]
	}


star: '*' | 'const' '*'
        
modifier: 'const' | 'struct' | 'enum' | 'unsigned' | 'long' | 'extern' | 'static' | 'short' | 'signed'

modmodifier: 'const' | 'struct' | 'enum' | 'extern' | 'static'

attribute: 'extern' | 'static' 

# IDENTIFIER: /[a-z]\w*/i
IDENTIFIER: /\w+/

TYPE: /\w+/

anything_else: /.*/
