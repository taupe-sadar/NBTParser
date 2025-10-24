#include <vector>
#include <string>
#include <memory>
#include <map>
#include <set>

inline std::string indentStr(int d)
{
  std::string indent;
  for(int i = 0; i < d; i++)
    indent += "  ";
  return indent;
}

struct Tag
{
  Tag(std::string const& _name, int _id):name(_name),id(_id){}

  virtual char getByte() const { throw std::runtime_error("Invalid access"); };
  virtual short getShort() const { throw std::runtime_error("Invalid access"); };
  virtual int getInt() const { throw std::runtime_error("Invalid access"); };
  virtual long getLong() const { throw std::runtime_error("Invalid access"); };
  virtual float getFloat() const { throw std::runtime_error("Invalid access"); };
  virtual double getDouble() const { throw std::runtime_error("Invalid access"); };
  virtual std::vector<unsigned char> getByteArray() const { throw std::runtime_error("Invalid access"); };
  virtual std::string getString() const { throw std::runtime_error("Invalid access"); };
  virtual std::vector<int> getIntArray() const { throw std::runtime_error("Invalid access"); };
  virtual std::vector<long> getLongArray() const { throw std::runtime_error("Invalid access"); };

  virtual Tag const * getMember(std::string const & element) const { throw std::runtime_error("Invalid access"); };
  virtual int size() const { throw std::runtime_error("Invalid access"); };
  virtual std::set<std::string> elements() const { throw std::runtime_error("Invalid access"); };

  virtual std::string str(int depth) const = 0;
  ~Tag(){};
  
  std::string name;
  int id;
};

struct TagEnd : public Tag
{
  TagEnd():Tag("",0){}
  std::string str(int depth) const { return ""; }
};

struct TagByte : public Tag
{
  TagByte(std::string const& name, char _val):Tag(name,1),val(_val){}
  char val;
  char getByte() const {return val;};
  std::string str(int depth) const { return indentStr(depth) + name + "(B) : " + std::to_string(val) + "\n"; }
};

struct TagShort : public Tag
{
  TagShort(std::string const& name, short _val):Tag(name,2),val(_val){}
  short val;
  short getShort() const {return val;};
  std::string str(int depth) const { return indentStr(depth) + name + "(W) : " + std::to_string(val) + "\n"; }
};

struct TagInt : public Tag
{
  TagInt(std::string const& name, int _val):Tag(name,3),val(_val){}
  int val;
  int getInt() const {return val;};
  std::string str(int depth) const { return indentStr(depth) + name + "(I) : " + std::to_string(val) + "\n"; }
};

struct TagLong : public Tag
{
  TagLong(std::string const& name, long _val):Tag(name,4),val(_val){}
  long val;
  long getLong() const {return val;};
  std::string str(int depth) const { return indentStr(depth) + name + "(L) : " + std::to_string(val) + "\n"; }
};

struct TagFloat : public Tag
{
  TagFloat(std::string const& name, float _val):Tag(name,5),val(_val){}
  float val;
  float getFloat() const {return val;};
  std::string str(int depth) const { return indentStr(depth) + name + "(F) : " + std::to_string(val) + "\n"; }
};

struct TagDouble : public Tag
{
  TagDouble(std::string const& name, double _val):Tag(name,6),val(_val){}
  double val;
  double getDouble() const {return val;};
  std::string str(int depth) const { return indentStr(depth) + name + "(D) : " + std::to_string(val) + "\n"; }
};

struct TagByteArray : public Tag
{
  TagByteArray(std::string const& name):Tag(name,7){}
  std::vector<unsigned char> list;
  std::vector<unsigned char> getByteArray() const {return list;};
  std::string str(int depth) const { return indentStr(depth) + name + " : (ByteArray) x" + std::to_string(list.size()) + "\n"; }
};

struct TagString : public Tag
{
  TagString(std::string const& name, std::string const & _val):Tag(name,8),val(_val){}
  std::string val;
  std::string getString() const {return val;};
  std::string str(int depth) const { return indentStr(depth) + name + "(S) : " + val + "\n"; }
};

struct TagList : public Tag
{
  TagList(std::string const& name, int id):Tag(name,9),list_id(id){}
  int list_id;
  std::vector<std::unique_ptr<Tag>> list;

  Tag const * getMember(std::string const & element) const;
  int size() const {return list.size();};
  
  std::string str(int depth) const
  {
    std::string s = indentStr(depth) + "(L) " + name + " :\n";
    for(int i = 0; i < list.size(); i++)
      s += list[i]->str(depth+1);
    return s; 
  }
};

struct TagCompound : public Tag
{
  TagCompound(std::string const& name):Tag(name,10){}
  std::map<std::string,std::unique_ptr<Tag>> compound;

  Tag const * getMember(std::string const & element) const;
  std::set<std::string> elements() const;

  std::string str(int depth) const
  { 
    std::string s = indentStr(depth) + "(C) " + name + " :\n";
    for(auto & c: compound)
      s += c.second->str(depth+1);
    return s; 
  }
};

struct TagIntArray : public Tag
{
  TagIntArray(std::string const& name):Tag(name,11){}
  std::vector<int> list;
  std::vector<int> getIntArray() const {return list;};
  std::string str(int depth) const { return indentStr(depth) + name + " : (IntArray) x" + std::to_string(list.size()) + "\n"; }
};

struct TagLongArray : public Tag
{
  TagLongArray(std::string const& name):Tag(name,12){}
  std::vector<long> list;
  std::vector<long> getLongArray() const {return list;};
  std::string str(int depth) const { return indentStr(depth) + name + " : (LongArray) x" + std::to_string(list.size()) + "\n"; }
};

std::unique_ptr<Tag> nbt_parse(std::vector<unsigned char> const & buf);
Tag const * getElement(Tag const * tag, std::string const & path);
