#include <iostream>
#include "nbt.h"

short shortBE(unsigned char const * buf)
{
  return(short(buf[0])<<8) +
        buf[1];
}
int intBE(unsigned char const * buf)
{
  return(int(buf[0])<<24) +
        (int(buf[1])<<16) +
        (int(buf[2])<<8) +
        buf[3];
}
long longBE(unsigned char const * buf)
{
  return (long(buf[0])<<56) +
         (long(buf[1])<<48) +
         (long(buf[2])<<40) +
         (long(buf[3])<<32) +
         (long(buf[4])<<24) +
         (long(buf[5])<<16) +
         (long(buf[6])<<8) +
         buf[7];
}

std::unique_ptr<Tag> nbt_parse(std::vector<unsigned char> const & buf, int &idx);

std::unique_ptr<Tag> parse_payload(std::vector<unsigned char> const & buf, int &idx, int id, std::string const & name)
{
  switch(id)
  {
    case 0:
      return std::unique_ptr<Tag>(new TagEnd());
    case 1:
      return std::unique_ptr<Tag>(new TagByte(name,buf[idx++]));
    case 2:
    {
      short val = shortBE(&buf[idx]);
      idx += 2;
      return std::unique_ptr<Tag>(new TagByte(name,val));
    }
    case 3:
    {
      int val = intBE(&buf[idx]);
      idx += 4;
      return std::unique_ptr<Tag>(new TagInt(name,val));
    }
    case 4:
    {
      long val = longBE(&buf[idx]);
      idx += 8;
      return std::unique_ptr<Tag>(new TagLong(name,val));
    }
    case 5:
    {
      int vint = intBE(&buf[idx]);
      float val = *((float*)&vint);
      idx += 4;
      return std::unique_ptr<Tag>(new TagFloat(name,val));
    }
    case 6:
    {
      int vint = longBE(&buf[idx]);
      double val = *((double*)&vint);
      idx += 8;
      return std::unique_ptr<Tag>(new TagDouble(name,val));
    }
    case 7:
    {
      TagByteArray* tagArray = new TagByteArray(name);
      int arraySize= intBE(&buf[idx]);
      idx += 4;
      tagArray->list.resize(arraySize);
      for(int i = 0 ; i < arraySize; i++)
        tagArray->list[i] = buf[idx++];
      return std::unique_ptr<Tag>(tagArray);
    }
    case 8:
    {
      unsigned short size= shortBE(&buf[idx]);
      idx += 2;
      std::string str(&buf[idx],&buf[idx+size]);
      idx += size;
      return std::unique_ptr<Tag>(new TagString(name,str));
    }
    case 9:
    {
      char listId = buf[idx++];
      TagList* tagList = new TagList(name,listId);
      int listLength= intBE(&buf[idx]);
      idx+=4;
      for(int i = 0 ; i < listLength; i++)
      {
        tagList->list.push_back(std::move(parse_payload(buf,idx,listId,name + "_" + std::to_string(i))));
        if(tagList->list.back() == nullptr)
          return nullptr;
      }
      return std::unique_ptr<Tag>(tagList);
    }
    case 10:
    {
      TagCompound* tagCompound = new TagCompound(name);
      std::unique_ptr<Tag> tag = nbt_parse(buf,idx);
      if(tag == nullptr)
          return nullptr;
      int tagId = tag->id;
      tagCompound->compound[tag->name] = std::move(tag);
      while(tagId != 0)
      {
        tag = nbt_parse(buf,idx);
        if(tag == nullptr)
          return nullptr;
        tagId = tag->id;
        if( tagCompound->compound.count(tag->name) > 0 )
        {
          std::cout << "Cannot add 2 tags with smae name in compound" << std::endl;
          return nullptr;
        }
        tagCompound->compound[tag->name] = std::move(tag);
      }
      return std::unique_ptr<Tag>(tagCompound);
    }
    case 11:
    {
      TagIntArray* tagIntArray = new TagIntArray(name);
      int arraySize = intBE(&buf[idx]);
      idx+=4;
      tagIntArray->list.resize(arraySize);
      for(int i = 0 ; i < arraySize; i++)
      {
        tagIntArray->list[i]= intBE(&buf[idx]);
        idx+=4;
      }
      return std::unique_ptr<Tag>(tagIntArray);
    }
    case 12:
    {
      TagLongArray* tagLongArray = new TagLongArray(name);
      int arraySize = intBE(&buf[idx]);
      idx+=4;
      tagLongArray->list.resize(arraySize);
      for(int i = 0 ; i < arraySize; i++)
      {
        tagLongArray->list[i]= longBE(&buf[idx]);
        idx+=8;
      }
      return std::unique_ptr<Tag>(tagLongArray);
    }
    default:
    {
      std::cout << "Unknown tag : " <<  id << std::endl;
      return nullptr;
    }
  }
}

std::unique_ptr<Tag> nbt_parse(std::vector<unsigned char> const & buf, int &idx)
{
  int id = buf[idx++];
  short nameSize = 0;
  std::string name;
  if( id != 0 )
  {
    nameSize = (buf[idx]<<8) + buf[idx+1];
    idx += 2;
    name = std::string(&buf[idx],&buf[idx+nameSize]);
    idx += nameSize;
  }
  return parse_payload(buf,idx,id,name);
}

std::unique_ptr<Tag> nbt_parse(std::vector<unsigned char> const & buf)
{
  int startIdx = 0;
  std::unique_ptr<Tag> tag(nbt_parse(buf,startIdx));
  if( startIdx != buf.size() )
    std::cout << "Warning : all the chunk data was not read ! (" << startIdx << "/" << buf.size() << ")" << std::endl;
  return std::move(tag);
}

Tag const * TagList::getMember(std::string const & element) const
{
  try
  {
    int idx = std::stoi(element);
    if( std::to_string(idx) != element )
      throw std::runtime_error("Invalid number as index");
    if( idx >= list.size() )
      throw std::runtime_error("List is too small");
    return list.at(idx).get();
  }
  catch(std::exception const & err)
  {
    throw std::runtime_error("Invalid member " + element + " : "  + err.what());
  }
}

Tag const * TagCompound::getMember(std::string const & element) const
{
  if(compound.count(element) > 0 )
    return compound.at(element).get();
  else
    throw std::runtime_error("Invalid member " + element);
}

std::set<std::string> TagCompound::elements() const
{
  std::set<std::string> elts;
  for(auto & e:compound)
    elts.insert(e.first);
  return elts;
};

Tag const * getElement(Tag const * tag, std::string const & path)
{
  std::string str = path;
  Tag const * t = tag;
  std::size_t found = str.find("/");
  while (found != std::string::npos)
  {
    t = t->getMember(str.substr(0,found));
    str = str.substr(found+1);
    found = str.find("/");
  }
  return t->getMember(str);
}
