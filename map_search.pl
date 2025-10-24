use strict;
use warnings;
use Data::Dumper;
use Cwd qw(getcwd);

my($minecraft_save)="/cygdrive/c/Users/Remi/AppData/Roaming/.minecraft/saves/Comme la demo";
my($nbtutil)="/cygdrive/c/projets/NBTExplorer/NBTUtil.exe";

my($win_path)="c:\\projets\\NBTParser\\output";

chdir($minecraft_save);

# my($nbt_path)="region/r.2.11.mca";
# my($nbt_path)="";
my($nbt_path)="data/map_0.dat";

system("$nbtutil","--path=$nbt_path","--json=$win_path/tmp.json" );
# system("$nbtutil","--path=$nbt_path","--printtree" );


