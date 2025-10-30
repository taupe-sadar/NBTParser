use strict;
use warnings;
use Data::Dumper;
use List::Util qw(min max);
use POSIX;

my($config)= read_conf("confs/chunk_trace.conf");

my($save_dir)=$$config{'args'}{'save_dir'};
my($dimension)=$$config{'args'}{'dimension'};
my(@center_block)=split(',',$$config{'args'}{'center_block'});
my(@size)=split(',',$$config{'args'}{'size'});

# print Dumper \@ARGV;<STDIN>;

if( $#ARGV <= 7 && $#ARGV >=4 )
{
  $save_dir = $ARGV[0];
  $dimension = $ARGV[1];
  @center_block = @ARGV[2..4];
  @size = @ARGV[5..$#ARGV];
  $size[0] = 20 if( $#ARGV < 5 );
  $size[1] = int($size[0]/2) if( $#ARGV < 6 );
  $size[2] = $size[0] if( $#ARGV < 7 );
}

my(@start_block)=(
  $center_block[0]-int($size[0]/2),
  $center_block[1]-int($size[1]/2),
  $center_block[2]-int($size[2]/2)
);

my($legend)=$$config{"legend"};

my(%only_show)=(
  # "stone" => 1,
  # "grass_block" => 1,
  # "dirt" => 1,
  # "air" => 1,
  # "cave_air" => 1,
  # "diorite" => 1,
  # "andesite" => 1,
  # "gravel" => 1,
  # "stone" => 1,
  # "infested_stone"=>1
);

##################################
# legend management

my(@guessed_chars)=funky_chars();
my($guess_idx)=0;

my(%legend_count)=();
my(%used_chars)=();
foreach my $b (keys(%$legend))
{
  $legend_count{$b} = 0;
  $used_chars{$$legend{$b}}=1;
}

my(@kf)=(keys(%only_show));
my($filter)=$#kf>=0;

##################################

my(@end_block)=();
for(my($i)=0;$i<=$#size;$i++)
{
  push(@end_block,$start_block[$i] + $size[$i] - 1);
}

my(@chunk_start)=(floor($start_block[0]/16),floor($start_block[2]/16));
my(@chunk_end)=(floor($end_block[0]/16),floor($end_block[2]/16));

my(@region_start)=(floor($chunk_start[0]/32),floor($chunk_start[1]/32));
my(@region_end)=(floor($chunk_end[0]/32),floor($chunk_end[1]/32));

my($num_levels)=$end_block[1]-$start_block[1]+1;
my(@regions_strings)=();
for(my($y)=0;$y<$num_levels;$y++)
{
  my(@region_level)=();
  for(my($cz)=$start_block[2];$cz<=$end_block[2];$cz++)
  {
    push(@region_level,"");
  }
  push(@regions_strings,\@region_level);
}

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
    
    my($palette,$chunks_lines)=nbt_parser($save_dir,$dimension,$rx,$rz,\@chunks,$start_block[1],$end_block[1]);
    # print Dumper $palette; <STDIN>;
    
    my($idx)=0;
    
    for(my($cz)=$min_z_chunk;$cz<=$max_z_chunk;$cz++)
    {
      my($z_min) = max(($rz*32 + $cz)*16,$start_block[2])%16;
      my($z_max) = min(($rz*32 + $cz)*16 +15,$end_block[2])%16;
      for(my($cx)=$min_x_chunk;$cx<=$max_x_chunk;$cx++)
      {
        my($x_min) = max(($rx*32 + $cx)*16,$start_block[0])%16;
        my($x_max) = min(($rx*32 + $cx)*16 +15,$end_block[0])%16;
        
        my($chunk_data)=$$chunks_lines[$idx++];
        
        my($zidx)=$z_min + 16*($cz + $rz*32) - $start_block[2];
        for(my($y)=0;$y<$num_levels;$y++)
        {
          my($chunk_level) = transform($$chunk_data[$y],$palette,[$x_min,$x_max],[$z_min,$z_max]);
          for(my($z)=0;$z<=$#$chunk_level;$z++)
          {
            $regions_strings[$y][$zidx + $z] .= $$chunk_level[$z];
          }
        }
      }
    }
  }
}

foreach my $b (sort(keys(%$legend)))
{
  if($legend_count{$b} > 0 )
  {
    print "$$legend{$b} : $b (x$legend_count{$b})\n";
  }
}

my($left_m)=5;
for(my($y)=0;$y<$num_levels;$y++)
{
  print "-"x(($#{$regions_strings[$y]}+1)*2)."\n";
  print "Level ".($start_block[1]+$y)."\n" ;
  my(@xlines)=xlabel($start_block[0],$size[0],$left_m);
  print $xlines[0]."\n";
  print $xlines[1]."\n";
  for(my($z)=0;$z<=$#{$regions_strings[$y]};$z++)
  {
    my($zcoord)=$start_block[2]+$z;
    my($zstr)="";
    if($zcoord%10 == 0)
    {
      $zstr = sprintf("%${left_m}s",$zcoord);
    }
    else
    {
      $zstr = ' 'x$left_m;
    }
    print "$zstr $regions_strings[$y][$z]\n";
  }
}

sub xlabel
{
  my($first,$num,$left_margin)=@_;
  
  my($mod)=$first%10;
  my($offset)=($mod>0)?(10-$mod):0;
  
  my($num_patterns)=int(($num-$offset)/10) + (($num%10 == $offset)?0:1);
  my($coord_str)=' 'x($offset*2 + $left_margin +1);
  my($dot)=' 'x($offset*2 + $left_margin + 1);
  for(my($i)=0;$i<$num_patterns;$i++)
  {
    $coord_str.=sprintf("%-20s",$first + $offset +$i*10);
  }
  $dot.=('|'.(' 'x19))x($num_patterns);
  return ($coord_str,$dot);
}

sub nbt_parser
{
  my($folder,$dim,$x,$z,$rchunks,$ymin,$ymax)=@_;
  my($path)=$folder;
  $path.= "/region" if($dim eq "normal");
  $path.= "/DIM-1/region" if($dim eq "nether");
  $path.= "/DIM1/region" if($dim eq "end");
  $path.="/r.$x.$z.mca";
  
  my(%palette)=();
  my(@chunk_lines)=();
  
  # print "$path\n";
  if( -e $path )
  {
    my($chunk_list)=join(" ",map {join(" ",@$_)} @$rchunks);
    
    my($path_for_c)=$path =~s/ /\\ /gr;
    my($command)="./nbt_parser chunk_blocks $path_for_c $ymin $ymax $chunk_list";
    # print "$command\n";
    # <STDIN>;

    open(NBT_PARSER, "$command |") or die "Cannot launch nbt_parser";
    my($line)="";
    my($chunk_lines)=$ymax - $ymin + 1;
    my($rcurrent_lines)=[];
    while(defined($line=<NBT_PARSER>))
    {
      chomp($line);
      if($line=~m/^minecraft\:(.*)\:(\d+)$/)
      {
        $palette{$2}=$1;
      }
      else
      {
        push(@$rcurrent_lines,$line);
        if($#$rcurrent_lines+1 == $chunk_lines)
        {
          push(@chunk_lines,$rcurrent_lines);
          $rcurrent_lines=[];
        }
      }
    }
    close(NBT_PARSER);
  }
  else
  {
    for(my($c)=0;$c<=$#$rchunks;$c++)
    {
      my(@chunk_line)=();
      for(my($y)=$ymin;$y<=$ymax;$y++)
      {
        push(@chunk_line,"0 "x256);
      }
      push(@chunk_lines,\@chunk_line);
    }
  }
  
  return (\%palette,\@chunk_lines);
}

sub transform
{
  my($level_line,$palette,$x_range,$z_range)=@_;
  my(@block_types)=split(' ',$level_line);
  my(@strings)=();
  for(my($z)=$$z_range[0];$z<=$$z_range[1];$z++)
  {
    my($raw)="";
    for(my($x)=$$x_range[0];$x<=$$x_range[1];$x++)
    {
      my($val)=$block_types[$z*16 + $x];
      
      $raw.=legend_value($val,$palette).' ';
    }
    push(@strings,$raw);
  }
  return \@strings;
}

sub legend_value
{
  use open ":std", ":encoding(UTF-8)";
  my($block_id,$palette)=@_;
  my($block)=$$palette{$block_id};
  
  unless(exists($$legend{$block}))
  {
    my($guess)=$guessed_chars[$guess_idx++];
    while(exists($used_chars{chr($guess)}))
    {
      $guess=$guessed_chars[$guess_idx++];
    }
    my($block_char)=chr($guess);
    $used_chars{$block_char} = 1;
    $$legend{$block} = $block_char;
    $legend_count{$block}=0;
  }
  $legend_count{$block}++;
  if($filter && !exists($only_show{$block}))
  {
    return '.';
  }
  else
  {
    return $$legend{$block};
  }
}

sub funky_chars
{
  my(@ascii)=(0x30..0x7d);
  my(@unicode_funky)=(0x2500..0x2570);
  return (@ascii,@unicode_funky);
}

sub read_conf
{
  my($file)=@_;
  my(%config)=("legend"=>{});
  open(CONF, "$file") or die "Cannot open $file";
  my(@lines)=(<CONF>);
  for my $line (@lines)
  {
    next if($line=~m/^#/);
    $line=~s/(\n|\r)//g;
    if( $line=~m/^(\S+)=(.*)$/ )
    {
      my($entry,$value)=($1,$2);
      if($entry =~m/^(.*):(.*)$/)
      {
        if($1 eq 'legend' || $1 eq 'args')
        {
          $config{$1}{$2} = $value;
        }
      }
    }
  }
  close(CONF);
  return \%config;
}

