#!/usr/bin/perl

use strict;
use warnings;
use Cwd;
use File::Spec;
use ExtUtils::MakeMaker;

WriteMakefile(
		NAME			=> 'File::Extract',
		ABSTRACT_FROM	=> 'pm/Extract.pm',
		VERSION_FROM	=> 'pm/Extract.pm',
		XSPROTOARG		=> '-noprototypes',
		MAN3PODS		=> {
			'pm/Extract.pm'	=> '$(INST_MAN3DIR)/File::Extract.$(MAN3EXT)'
		},
		PM				=> {
			'pm/Extract.pm'	=> '$(INST_LIBDIR)/Extract.pm'
		},
		INC				=> '-Wall -I'.cwd(),
		LIBS			=> '-lextractor',
		do {
			my @clean = ();
			my @OBJECT = ();
			my %XS = ();
			for my $xs (<xs/*.xs>) {
				(my $c = $xs) =~ s/\.xs$/\.c/i;
				(my $o = $xs) =~ s/\.xs$/\$(OBJ_EXT)/i;
				$XS{$xs} = $c;
				push @OBJECT, $o;
				push @clean, $o;
			}

			for my $c (<*.c>) {
				(my $o = $c) =~ s/\.c$/\$(OBJ_EXT)/i;
				push @OBJECT, $o;
				push @clean, $o;
			}

			clean => { FILES => join(" ", @clean) }, OBJECT => join(" ", @OBJECT), XS => \%XS
		},
);

package MY;

sub const_cccmd {
	my $inherited = shift->SUPER::const_cccmd(@_);
	return '' unless $inherited;
	use Config;
	if ($Config{cc} eq 'cl') {
		warn "you are using MSVC... my condolences.";
		$inherited .= ' /Fo$@';
	} else {
		$inherited .= ' -o $@';
	}

	return $inherited;
}

1;
