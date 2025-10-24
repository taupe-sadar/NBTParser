use strict;
use warnings;
use Data::Dumper;
use Cwd qw(getcwd);

#Labo des couleurs = 517

my(@small_maps)=(
93, 84, 79, 257,
98, 136, 74, 241,
125, 70, 28, 229,
181, 189, 211, 218
);

my(@big_maps)=(
  0,   0,   0, 613, 608,   0,   0,   0,
  0, 506, 501, 496, 491, 618, 623, 628,
  0, 511, 455, 271, 276, 464, 603, 633,
  0, 516, 446, 159, 237, 369, 598, 638,
  0, 522, 429, 167, 172, 386, 593, 643,
710, 527, 482, 412, 395, 477, 588, 648,
700, 532, 537, 542, 547, 552, 583, 653,
705, 690, 685, 672, 573, 578, 667, 662,
);


my($minecraft_save)='/cygdrive/c/Users/Remi/AppData/Roaming/.minecraft/saves/Comme\ la\ demo';
my($minecraft_save_perl)='/cygdrive/c/Users/Remi/AppData/Roaming/.minecraft/saves/Comme la demo';

my($base_dir)="./maps";

my(%selected_maps)=();
if( $#ARGV >= 0)
{
  for(my($i)=0;$i<=$#ARGV;$i++)
  {
    if( $ARGV[$i] =~ m/^(\d+)$/ )
    {
      $selected_maps{$ARGV[$i]}=1;
    }
    elsif($ARGV[$i] eq "small")
    {
      for(my($j)=0;$j<=$#small_maps;$j++)
      {
        $selected_maps{$small_maps[$j]}=1 unless $small_maps[$j] == 0;
      }
    }
    elsif($ARGV[$i] eq "big")
    {
      for(my($j)=0;$j<=$#big_maps;$j++)
      {
        $selected_maps{$big_maps[$j]}=1 unless $big_maps[$j] == 0;
      }
    }
  }
}
 
if(keys(%selected_maps))
{
  foreach my $k (keys(%selected_maps))
  {
    process("$minecraft_save/data","map_$k.dat","$base_dir/colors_$k.txt");
  }
}
else
{
  opendir(DIR, $minecraft_save_perl."/data") or die "cannot open $minecraft_save/data";
  my($mapfile)="";
  my(@files)=();
  while($mapfile = readdir(DIR))
  {
    if($mapfile=~/map_(\d+).dat/)
    {
      push(@files,$mapfile);
    }
  }
  close DIR;

  @files = sort fn @files;

  foreach my $file (@files)
  {
    process("$minecraft_save/data",$file);
  }
}

sub process
{
  my($dir,$file,$print_map)=@_;
  my($data)=extract_map($dir,$file);
  next if $#$data < 0;
  
  if(defined($print_map))
  {
    print_map($data,$print_map);
  }    
  my($map) = stats_map($data);
  # print "$file : $$map[0] $$map[1]\n";
  
  # print Dumper \@vals;
  if( $#{$$map[0]} >= 0 )
  {
    print "$file : Zeros (".($#{$$map[0]}+1)."):\n";
    my($nb)=0;
    foreach my $zero (@{$$map[0]})
    {
      print "  [ $$zero[0], $$zero[1] ]\n";
      last if( $nb++ > 9 );
      
    }
    
  }
  
  
  if( $$map[1] > 0 )
  {
    print "Found seeked ones !\n";
    <STDIN>;
  }
  
}

sub fn
{
  return (mapid($a) <=> mapid($b));
}

sub mapid
{
  my($s)=@_;
  return $1 if($s=~/map_(\d+).dat/);
  return -1;
}

sub extract_map
{
  my($dir, $file)=@_;
  if($file=~/map_(\d+).dat/)
  {
    my($id)=$1;
    print "Map : $file ...\n";
    
    my($command)="./nbt_parser map $dir/$file";
    open(NBT_PARSER, "$command |") or die "Cannot open nbt_parser output";
    my($line)=<NBT_PARSER> or die "Empty output in nbt_parser";
    close(NBT_PARSER);
    
    my(@vals)=split(" ",$line);
    return \@vals;
  }
  return [];
}

sub print_map
{
  my($rvals,$filename)=@_;
  my($n)=0;

  my($byte)=0;
  my($count)=0;
  open( MAP , '>', $filename) or die $filename;
  for(my($i)=0;$i<=$#$rvals;$i++)
  {
    print MAP sprintf("%02X ",$$rvals[$i]%256);
    print MAP "\n" if( $i%128 == 127 );
  }
  close MAP;
}


sub stats_map
{
  my($rvals)=@_;
  my($zeros)=[];
  my($seeked)=0;
  my($large)=0;
  for(my($i)=0;$i<=$#$rvals;$i++)
  {
    if($$rvals[$i]==0x00)
    {
      push(@$zeros,[int($i/128),$i%128]);
    }
    elsif(1)
    {
    }
    else
    {
      $seeked++;
      print "$i --> ".sprintf("0x%02X",$$rvals[$i])."\n";
    }
  }
  return [$zeros,$seeked];  
}
