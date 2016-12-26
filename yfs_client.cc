// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <list>

using namespace std;

yfs_client::yfs_client()
{
    ec = new extent_client();

}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client();
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
yfs_client::issym(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_SYM) {
        printf("isfile: %lld is a sym\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}

bool
yfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}

int
yfs_client::getsym(inum inum, syminfo &sym)
{
    int r = OK;

    printf("getsym %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    sym.atime = a.atime;
    sym.mtime = a.mtime;
    sym.ctime = a.ctime;
	sym.size = a.size;
	printf("getfile %016llx -> sz %llu\n", inum, sym.size);
release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
	int i;
	string buf;
	string new_buf;
	ec->get(ino,buf);
	for(i = 0; i < size; i++)
	{
		if(i<buf.size())
			new_buf += buf[i];
		else
			new_buf += '\0';
	}
	ec->put(ino,new_buf);
    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
	bool found;
	lookup(parent,name,found,ino_out);
	if(found) return r;
	
	string name_out(name);
	ec->create(2,ino_out);
	
	string buf;
	ec->get(parent,buf);
	buf = buf + name_out + '\0' + filename(ino_out)+ '\0';
	ec->put(parent,buf);
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
	bool found;
	lookup(parent,name,found,ino_out);
	if(found) return r;
	
	string name_out(name);
	ec->create(1,ino_out);
	
	string buf;
	ec->get(parent,buf);
	buf = buf + name_out + '\0' + filename(ino_out)+ '\0';
	ec->put(parent,buf);
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
	found = false;
	string name_out(name);
	std::list<dirent> list;
	readdir(parent,list);
	
	std::list<dirent>::iterator ite;
	for(ite = list.begin(); ite!= list.end(); ite++)
		if(ite->name == name_out)
		{
			ino_out =ite->inum;
			found = true;
			break;
		}
	
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
	dirent tmp;
	string buf;
	string file_name = "";
	string file_ino = "";
	ec->get(dir,buf);
	list.empty();
	int i;
	for(i = 0; i < buf.size(); i++)
	{
		if(buf[i] != '\0')
			file_name += buf[i];
		else
		{
			tmp.name = file_name;
			i++;
			while(buf[i] != '\0')
			{
				file_ino += buf[i];
				i++;
			}
			tmp.inum = n2i(file_ino);
			list.push_back(tmp);
			file_name = file_ino = "";
		}
	}
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */
	string buf;
	ec->get(ino, buf);
	data = "";
	int i;
	for(i = 0; i<size; i++)
	{
		if(i+off<buf.size())
			data+=buf[i+off];
		else break;
	}
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
	string buf;
	ec->get(ino,buf);
	int ori_buff_size = buf.size();
	int i;
	bytes_written = size;
	
	if(off>=ori_buff_size)
	{
		for(i = 0; i+ori_buff_size < off; i++)
			buf+='\0';
		bytes_written += off-ori_buff_size;
		ori_buff_size = buf.size();
	}
	
	for(i = 0;i < size; i++)
	{
		if(i+off<ori_buff_size)
			buf[i+off] = data[i];
		else
			buf+=data[i];
	}
	ec->put(ino,buf);
	
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
	bool found;
	inum ino_out;
	string name_out(name);
	lookup(parent,name,found,ino_out);
	if(!found) return r;
	
	ec->remove(ino_out);
	
	std::list<dirent> list;
	string buf;
	readdir(parent,list);
	
	std::list<dirent>::iterator ite;
	for(ite = list.begin(); ite!= list.end(); ite++)
		if(ite->name != name_out)
			buf = buf + ite->name + '\0' + filename(ite->inum) + '\0';
			
	ec->put(parent,buf);
	
    return r;
}

int yfs_client::mksym(const char *link, inum parent, const char *name, inum &ino_out)
{
    int r = OK;
    std::fstream ofs("logsymlk.me", std::ios::out);
    bool found = false;
    std::string buf;
    lookup(parent, name, found, ino_out);
    if (found == true) return r;

    ec->create(extent_protocol::T_SYM, ino_out);
    ec->get(parent, buf);
    std::string name_out(name);
    
    buf = buf+name_out+'\0'+filename(ino_out)+'\0';
    ec->put(parent, buf);
    ofs << buf << "\n";

    buf = "";
    int off = 0;
    while (link[off] != '\0')
    {
        buf += link[off];
        off++;
    }
    ofs << buf << "\n";
    ec->put(ino_out, buf);
    return r;
}

int yfs_client::readsym(inum ino, std::string &result)
{
    std::fstream ofs("logReadLk.me", std::ios::out);
    int r = OK;
    ec->get(ino, result);
    ofs << result << "\n";
    return r;
}
