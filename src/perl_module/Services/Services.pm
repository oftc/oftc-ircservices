package Services;

use strict;
use Carp;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);

require Exporter;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Services ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
@EXPORT = qw(	
);

$VERSION = '0.01';

require XSLoader;
XSLoader::load('Services', $VERSION);

# Preloaded methods go here.

1;
__END__
