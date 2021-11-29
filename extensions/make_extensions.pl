#!/usr/bin/perl -w
# extracts token table data and creates fake extensions that listamos can use
use strict;

main();
sub main {
    if (@ARGV) {
        for my $ext (@ARGV) {
            print "$ext: slot ".slot($ext)."\n";
            map { $_->[1] =~ s/[\x80-\xFF]/chr(ord($&)-0x80).">> <<"/e;
                  $_->[1] =~ s/[\xFD-\xFF]$//;
                  $_->[1] =~ s/[\x00-\x1F\x80-\xFF]/sprintf '\x%02X', ord $&/eg;
                  printf "  %04X <<%s>>\n", @{$_} } table($ext);
        }
        exit;
    }
    # read the token tables inside AMOS 1.3 and AMOS Professional.
    # Merge them together and remove dummy instructions 0x2562 and 0x293A
    my %seen = (0x2562 => 1, 0x293A => 1);
    save_as_c('00_base.h', 'ext00_base', fake_extension(0,
        grep {!$seen{$_->[0]}++}
        (table('base/AMOS-V1.36', 0xEE46, 0xED84),
         table('base/AMOSPro.Lib-V2.00', 0xC40, 0xC94))));

    # convert the four default extensions
    save_as_c('01_music.h', 'ext01_music',
        fake_extension(1, table('base/AMOSPro_Music.Lib-V2.00')));
    save_as_c('02_compact.h', 'ext02_compact',
        fake_extension(2, table('base/AMOSPro_Compact.Lib-V2.00')));
    save_as_c('03_request.h', 'ext03_request',
        fake_extension(3, table('base/AMOSPro_Request.Lib-V2.00')));
    save_as_c('06_ioports.h', 'ext06_ioports',
        fake_extension(6, table('base/AMOSPro_IOPorts.Lib-V2.00')));

    # convert all third party extensions
    for my $ext (<exts/*>) {
        my $f = $ext; $f =~ s{exts/}{};
        my $slot = slot($ext);
        $slot = 10 if $slot == -1 && $ext =~ /Dump/;
        $slot = 12 if $slot == -1 && $ext =~ /TURBO_Plus/;
        $slot = 14 if $slot == -1 && $ext =~ /Intuition/;
        die "$ext can't decide slot\n" if $slot == -1;
        save($f, fake_extension($slot, table($ext)));
    }
}

my $buf;
sub read_file {
    die "$_[0]: $!\n" unless open my $fh, '<', $_[0];
    die "$_[0]: $!\n" unless read($fh, $buf, 800000) && close($fh);
}
sub b { unpack 'C', substr $buf, $_[0], 1 }
sub w { unpack 'n', substr $buf, $_[0], 2 }
sub l { unpack 'N', substr $buf, $_[0], 4 }

sub table {
    my ($file, $table_offset, $i) = @_;
    read_file($file);

    if (! $table_offset) {
        $table_offset = l(32) + 32 + 18;
        $table_offset += 4 if l(32 + 18) == 0x41503230; # AP20
        $i = $table_offset;
    }

    my @out;
    while ($i < length($buf)) {
        last if w($i) == 0;
        my $offset = $i - $table_offset;
        $i += 4;
        my $len = 0;
        $len++ until b($i + $len) >= 0x80; $len++;
        $len++ until b($i + $len) >= 0xFD; $len++;
        push @out, [$offset, substr $buf, $i, $len];
        $i += $len + ($len & 1); # re-align to word boundary
    }
    return @out;
}

sub slot {
    my ($file) = @_;

    read_file($file);

    my $start = 32 + 18 + l(32) + l(36);
    my $end   = $start + l(40);
    my ($b, $w, $l) = (-1, -1, -1);
    for (my $i = $start; $i < $end; $i += 2) {
        my $c = w($i);
        last if $c == 0x4E75;
        $b = ($c & 0xFF) + 1 if $c >= 0x7000 && $c <= 0x7019;
        $w = w($i+2)     + 1 if $c == 0x303C && w($i+2) <= 25;
        $l = l($i+2)     + 1 if $c == 0x203C && l($i+2) <= 25;
    }
    return $b > 0 ? $b : $l > 0 ? $l : $w;
}

sub fake_extension {
    my ($slot, @table) = @_;

    my $tbl = '';
    my $off = $table[0]->[0];

    for my $i (@table) {
        # fill offset gaps by adding ":" to previous instruction's type
        $off++, $tbl =~ s/([\xFD\xFE\xFF]\x00?)$/:$1/ while $off < $i->[0];

        # write instruction, replacing pointers with "===="
        $tbl .= "====$i->[1]";
        $off += length($i->[1]) + 4;

        # word alignment
        $tbl .= "\0", $off++ if $off & 1;
    }
    # mark end of table
    $tbl .= "\0\0";

    $off = $table[0]->[0] < 0 ? -$table[0]->[0] : 0;
    return pack('N12n', 0x3F3,0,1,0,0,0,0x3E9,0,$off,length($tbl),4,0,0)
        . $tbl . pack('n2', 0x7000 | (($slot - 1) & 0xFF), 0x4E75);
}

sub save {
    my ($file, $data) = @_;
    die "$file: $!\n" unless open my $fh, '>', $file;
    die "$file: $!\n" unless print($fh $data) && close($fh);
}

sub save_as_c {
    my ($file, $name, $data) = @_;
    die "$file: $!\n" unless open my $fh, '>', $file;
    my $hex = join '', map { sprintf '0x%02X,', $_ } unpack 'C*', $data;
    $hex =~ s/(0x..,){12}/$&\n    /g;
    print $fh "unsigned char $name [] = {\n    $hex\n};\n";
    close($fh);
}
