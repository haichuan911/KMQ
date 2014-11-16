#ifndef _H_SECTIONREADER_
#define _H_SECTIONREADER_

#include "net.h"


KMQ_DECLARATION_START

class SectionReadWriter {
 public:
    SectionReadWriter() {
	rlen = rindex = wlen = windex = 0;
    }
    ~SectionReadWriter() {
    }

    int InitReader(uint32_t len) {
	rlen = len;
	rindex = 0;
	return 0;
    }
    
    int ResetReader() {
	rlen = rindex = 0;
	return 0;
    }

    int InitWriter(uint32_t len) {
	wlen = len;
	windex = 0;
	return 0;
    }
    int ResetWriter() {
	wlen = windex = 0;
	return 0;
    }

    int ReadSection(Conn *conn, char *rbuffer);
    int WriteSection(Conn *conn, char *wbuffer);
    
 private:
    uint32_t rlen, wlen;
    uint32_t rindex, windex;
};







}














#endif // _H_SECTIONREADER_
