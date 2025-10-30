use strict;
use warnings;
use Data::Dumper;


my($config)=read_conf("confs/diff_chunks.conf");
my($demo_dir)=$$config{'dir1'};
my($crea_dir)=$$config{'dir2'};

my($dimension,@starts_lengths)=(@ARGV);

my($start_length_str)=join(" ",@starts_lengths);

system("perl chunk_trace.pl '$demo_dir' $dimension $start_length_str > output/demo.txt");
system("perl chunk_trace.pl '$crea_dir' $dimension $start_length_str > output/crea.txt");
# print("perl chunk_trace.pl '$crea_dir' $dimension $start_length_str > output/crea.txt");

my(@ps)=`ps | grep WinMerge`;
if( $#ps < 0)
{
  system("/cygdrive/c/Program\ Files\ \(x86\)/WinMerge/WinMergeU.exe"," output/demo.txt"," output/crea.txt") unless(fork());
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