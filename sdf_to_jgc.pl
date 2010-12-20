#!/usr/bin/perl
#takes an SDF file, which stores the elevations as character strings seperated by newline characters
#and packs it into a fixed length (1200*1200+4)*16bit file ~ 2.8 MB instead of ~7 MB of randomness.

#highest elevation in the world is 8850 meters, so 16 bits should be plenty. (max is 65536 meters).

use IO::File;

foreach (@ARGV) {
    @fields=split(/\./,$_);
    $in_filename=$fields[0];
    print "Doing $in_filename\n";
    build_index($in_filename);
    print "Done with $in_filename\n";
}

sub build_index {
    my $filename = shift;

    open INFILE, "$filename.sdf", or die $!;
    open OUTFILE, ">$filename.jgc", or die $!;

    my @file=<INFILE>;

    foreach(@file){
        my $line=$_;
        chomp($line);
        my $code=pack(S,$line);
        print OUTFILE "$code";
    }
    
    close INFILE;
    close OUTFILE;    
}        

