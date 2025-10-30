use strict;
use warnings;
use Data::Dumper;
use List::Util qw(min max);
use POSIX;

my($conf)=read_conf('confs/search_block.conf');

my($save_dir)=$$conf{'save_dir'};

die "search_block.pl dimension block_type X Y [lengthX lengthY] [limit]\n" if($#ARGV < 3 || $#ARGV > 6);

my($dimension) = $ARGV[0];
my($block_type)=$ARGV[1];
my(@center_block) = @ARGV[2..3];
my($limit_blocks) = 20;
if($#ARGV == 4 || $#ARGV == 6)
{
  $limit_blocks = $ARGV[-1];
}
my(@size) = (100,100);
if($#ARGV > 3)
{
  @size = @ARGV[4..5];
}

my(@start_block)=(
  $center_block[0]-int($size[0]/2),
  $center_block[1]-int($size[1]/2)
);


my(@end_block)=();
for(my($i)=0;$i<=$#size;$i++)
{
  push(@end_block,$start_block[$i] + $size[$i] - 1);
}


my(@chunk_start)=(floor($start_block[0]/16),floor($start_block[1]/16));
my(@chunk_end)=(floor($end_block[0]/16),floor($end_block[1]/16));

my(@region_start)=(floor($chunk_start[0]/32),floor($chunk_start[1]/32));
my(@region_end)=(floor($chunk_end[0]/32),floor($chunk_end[1]/32));

my($total_count,@total_blocks)=(0);

my($cumulative_limit)=$limit_blocks;
for(my($rz)=$region_start[1];$rz<=$region_end[1];$rz++)
{
  my($min_z_chunk)=max($rz*32,$chunk_start[1])%32;
  my($max_z_chunk)=min($rz*32+31,$chunk_end[1])%32;

  for(my($rx)=$region_start[0];$rx<=$region_end[0];$rx++)
  {
    my($min_x_chunk)=max($rx*32,$chunk_start[0])%32;
    my($max_x_chunk)=min($rx*32+31,$chunk_end[0])%32;

    my(@chunks)=();
    for(my($cz)=$min_z_chunk;$cz<=$max_z_chunk;$cz++)
    {
      for(my($cx)=$min_x_chunk;$cx<=$max_x_chunk;$cx++)
      {
        push(@chunks,[$cx,$cz]);
      }
    }
    
    my($count_blocks,$rblocks)=nbt_parser($save_dir,$dimension,$block_type,$rx,$rz,\@chunks,$cumulative_limit);
    $total_count += $count_blocks;
    push(@total_blocks,@$rblocks);

    $cumulative_limit -= $count_blocks;
  }
}

print "$total_count found, (".($#total_blocks+1)." displayed)\n";
for my $b (format_output(@total_blocks))
{
  print "$b\n";
}

sub format_output
{
  my(@block_coords)=@_;
  my(@str_blocks)=();
  my(@max_length)=(0,0,0);
  for(my($i)=0;$i<=$#block_coords;$i++)
  {
    my(@s_block)=map({"$_";} @{$block_coords[$i]});
    for(my($j)=0;$j<3;$j++)
    {
      $max_length[$j]=length($s_block[$j]) if($max_length[$j]<length($s_block[$j]));
    }
    push(@str_blocks,\@s_block);
  }
  my(@s_total_blocks)=();
  for my $b (@str_blocks)
  {
    my($str)="";
    for(my($j)=0;$j<3;$j++)
    {
      $str.=sprintf("%$max_length[$j]s",$$b[$j]);
      $str.=" " if($j<2);
    }
    push(@s_total_blocks,$str);
  }
  return @s_total_blocks;
}

sub nbt_parser
{
  my($folder,$dim,$block_type,$x,$z,$rchunks,$limit)=@_;
  my($path)=$folder;
  $path.= "/region" if($dim eq "normal");
  $path.= "/DIM-1/region" if($dim eq "nether");
  $path.= "/DIM1/region" if($dim eq "end");
  $path.="/r.$x.$z.mca";

  $limit= "" unless(defined($limit));

  my($count)=0;
  my(@blocks)=();
  if( -e $path )
  {
    my($chunk_list)=join(" ",map {join(" ",@$_)} @$rchunks);
    
    my($path_for_c)=$path =~s/ /\\ /gr;
    my($command)="./nbt_parser search $path_for_c minecraft:$block_type $chunk_list $limit";
    # print "r.$x.$z.mca\n";
    print "$command\n";
    # <STDIN>;
    open(NBT_PARSER, "$command |") or die "Cannot launch nbt_parser";
    my($line)="";
    
    my($line_idx)=0;
    while(defined($line=<NBT_PARSER>))
    {
      chomp($line);
      next unless( $line=~m/^(\d|\s)+$/);

      my(@coords)=split(" ",$line);
      if( $line_idx == 0 && $#coords == 0) 
      {
        $count += $coords[0];
        $line_idx++;
      }
      elsif( $#coords == 2 )
      {
        $coords[0] += 512 * $x;
        $coords[2] += 512 * $z;
        push(@blocks,\@coords);
        $line_idx++;
      }
    }
    close(NBT_PARSER);
  }
  return ($count,\@blocks);
}

sub read_conf
{
  my($file)=@_;
  my(%config)=();
  open(CONF, "$file") or die "Cannot open $file";
  my(@lines)=(<CONF>);
  for my $line (@lines)
  {
    next if($line=~m/^#/);
    $line=~s/(\n|\r)//g;
    if( $line=~m/^(\S+)=(.*)$/ )
    {
      $config{$1}=$2;
    }
  }
  close(CONF);
  return \%config;
}
