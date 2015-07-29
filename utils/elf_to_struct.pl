#!/usr/bin/env perl
use warnings;
use strict;

# elf_to_struct.pl -file <elf file with debuginfo> -struct Transmitter -struct Model -struct display_settings

use Data::Dumper;
use Getopt::Long;

my %structs;
my %ids;
my %names;
my %size;
my %array;

my $holes = 0;

sub process_elf {
    my($file) = @_;
    open my $fh, "-|", "objdump --dwarf $file 2>/dev/null" or die "Couldn't read $file\n";
    my @lines = <$fh>;
    my $idx = 0;
    while ($idx <= $#lines) {
        $_ = $lines[$idx];
        chomp;
        if(/^ <1><(\S+)>:.*DW_TAG_typedef/) {
            my $id = $1;
            while($idx < $#lines && $lines[$idx+1] =~ /^    /) {
                $_ = $lines[++$idx];
                if (/DW_AT_type\s*:\s*<0x(\S+)>/) {
                    $ids{$id} = $1;
                }
                elsif (/DW_AT_name\s+:.*\s(\S+)\s*$/) {
                    $names{$id} = $1;
                }
            }
        }
        elsif(/^ <1><(\S+)>:.*DW_TAG_base_type/) {
            my $id = $1;
            while($idx < $#lines && $lines[$idx+1] =~ /^    /) {
                $_ = $lines[++$idx];
                if (/DW_AT_byte_size\s*:\s*(\d+)/) {
                    $size{$id} = $1;
                }
            }
        }
        elsif(/^ <1><(\S+)>:.*DW_TAG_enumeration_type/) {
            my $id = $1;
            while($idx < $#lines && $lines[$idx+1] =~ /^    /) {
                $_ = $lines[++$idx];
                if (/DW_AT_byte_size\s*:\s*(\d+)/) {
                    $size{$id} = $1;
                }
            }
        }
        elsif(/^ <1><(\S+)>:.*DW_TAG_array_type/) {
            my $id = $1;
            while($idx < $#lines && $lines[$idx+1] !~ /^\s*<1>/) {
                $_ = $lines[++$idx];
                if (/DW_AT_type\s*:\s*<0x(\S+)>/) {
                    $array{$id}{type} = $1;
                    $array{$id}{size} ||= 0;
                }
                elsif (/<([0-9a-f]+)>\s*:.*DW_TAG_subrange_type/) {
                    while($idx < $#lines && $lines[$idx+1] =~ /^    /) {
                        $_ = $lines[++$idx];
                        if (/DW_AT_upper_bound\s+:.*\s(\d+)\s*$/) {
                            if ($array{$id}{size}) {
                                $array{$id}{size} *= $1+1;
                            } else {
                                $array{$id}{size} = $1+1;
                            }
                        }
                    }
                }
            }
        }
        elsif(/^ <1><(\S+)>:.*DW_TAG_structure_type/) {
            my $id = $1;
            $structs{$id} = [];
            while($idx < $#lines && $lines[$idx+1] !~ /^ <1>/) {
                $_ = $lines[++$idx];
                if (/DW_AT_name\s+:.*\s(\S+)\s*$/) {
                    #print "'$1': $id\n";
                    $names{$id} = $1;
                }
                elsif (/DW_AT_byte_size\s*:\s*(\d+)/) {
                    $size{$id} = $1;
                }
                elsif (/<([0-9a-f]+)>\s*:.*DW_TAG_member/) {
                    my $childid = $1;
                    push @{ $structs{$id} }, {name => "UNKNOWN", type => undef, offset => undef};
                    while($idx < $#lines && $lines[$idx+1] =~ /^    /) {
                        $_ = $lines[++$idx];
                        if (/DW_AT_name\s+:.*\s(\S+)\s*$/) {
                            $structs{$id}[-1]{name} = $1;
                        }
                        elsif (/DW_AT_type\s*:\s*<0x(\S+)>/) {
                            $structs{$id}[-1]{type} = $1;
                        }
                        elsif (/DW_AT_data_member_location\s*:\s*(\d+)/) {
                            $structs{$id}[-1]{offset} = $1;
                        }
                    }
                }
            }
        }
        $idx++;
    }
}
sub dereference_typedefs {
    for my $id (keys %ids) {
        next if ($structs{$id});
        next if ($size{$id});
        next if ($array{$id});
        my $nextid = $ids{$id};
        while(! $size{$nextid} && $ids{$nextid}) {
            $nextid = $ids{$nextid};
        };
        if ($size{$nextid}) {
            $size{$id} = $size{$nextid};
        }
    }
}

sub process_struct {
    my($struct, $var, $indent) = @_;
    $indent ||= "";
    $var ||= "";
    my($id) = grep { $names{$_} eq $struct } sort {hex($a) <=> hex($b)} keys %names;
    die "Couldn't find name '$struct'\n" if(! $id);
    die "Couldn't find struct '$struct'\n" if( ! $structs{$id});
    print "-- $struct:\n" if(! $var);
    my $expected_pos = 0;
    foreach my $ref (@{ $structs{$id} }) {
        my $name = $ref->{name};
        my $type = $ref->{type};
        my $offset = $ref->{offset};
        my $range = "";
        my $count = 1;

        if ($expected_pos != $offset) {
            print("    ${indent}--Hole detected of size: " . ($offset - $expected_pos) . "\n");
            $expected_pos = $offset;
            $holes++;
        }
        if($array{ $ref->{type} }) {
            my $size = $array{ $ref->{type} }{size};
            $range = "[$size]";
            $count = $size;
            $type = $array{ $ref->{type} }{type};
        }
        if (! $size{ $type}) {
            print("    $indent$var$name$range : Unknown($type)\n");
            continue;
        }
        my $size = $size{ $type} * $count;
        $expected_pos += $size;
        
        if($structs{ $type }) {
            print "    $indent" . "-- $var$name$range($offset): $names{$type}\n";
            for my $i (0 .. $count-1) {
                my $varname = "$var$name" . ($range ? "[$i]" : "") . ".";
                process_struct($names{ $type }, "$varname", "    $indent");
            }
        } elsif($size{ $type }) {
            print("    $indent$var$name$range($offset) : $size{ $type }\n");
        }
    }
    if ($expected_pos != $size{$id}) {
        print("    ${indent}--Hole detected of size: " . ($size{$id} - $expected_pos) . "\n");
        $holes++
    }
}

my $file;
my @structs;
GetOptions("file=s" => \$file, "struct=s" => \@structs);
process_elf($file);
dereference_typedefs();
#print "Names:\n"; foreach (sort { hex($a) <=> hex($b)} keys %names) { print "    $_ : $names{$_}\n"; }
#print "Ids:\n"; foreach (sort { hex($a) <=> hex($b) } keys %ids) { print "    $_ : $ids{$_}\n"; }
#print "Array:\n"; foreach (sort { hex($a) <=> hex($b) } keys %array) { print "    $_ : $array{$_}{size} - $array{$_}{type}\n"; }
foreach my $struct (@structs) {
    process_struct($struct);
}
exit(!!$holes);
