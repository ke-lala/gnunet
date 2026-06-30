package File::Extract;

use strict;
use warnings;
use DynaLoader;

our $VERSION = 0.01;
our @ISA = qw(DynaLoader);

sub dl_load_flags { 0x01 }

bootstrap File::Extract $VERSION;

sub new {
	my $class = shift;
	$class = ref $class || $class;
	my $self = {};
	bless $self, $class;
	$self->init();
	return $self;
}

1;

=pod

=head1 DESCRIPTION

File::Extract - Perl interface to libdoodle/libextractor

=cut
