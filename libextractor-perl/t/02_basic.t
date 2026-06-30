#!/usr/bin/perl

use strict;
use warnings;
use Test::More tests => 2;
use File::Extract;

my $extractor = File::Extract->new();
isa_ok($extractor, "File::Extract");

ok($extractor->loadDefaultLibraries());
