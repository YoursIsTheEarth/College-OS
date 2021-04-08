/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

void ContFramePool::assign_empty(unsigned long frame) {
	
	char masks[4] = {0x3f, 0xcf, 0xf3, 0xfc};
	
	unsigned long index = (frame) / 4;
	unsigned long offset = (frame) % 4;
	
	bitmap[index] = bitmap[index] & masks[offset];
}

void ContFramePool::assign_head(unsigned long frame) {
	
	char masks[4] = {0xbf, 0xef, 0xfb, 0xfe};
	
	unsigned long index = (frame) / 4;
	unsigned long offset = (frame) % 4;
	
	bitmap[index] = bitmap[index] & masks[offset];
	bitmap[index] = bitmap[index] | (HEAD >> offset * 2);
}

void ContFramePool::assign_alloc(unsigned long frame) {
	
	char masks[4] = {0x7f, 0xdf, 0xf7, 0xfd};
	
	unsigned long index = (frame) / 4;
	unsigned long offset = (frame) % 4;
	
	bitmap[index] = bitmap[index] & masks[offset];
	bitmap[index] = bitmap[index] | (ALLOCATED >> offset * 2);
}

void ContFramePool::assign_closed(unsigned long frame) {
	
	unsigned long index = (frame) / 4;
	unsigned long offset = (frame) % 4;
	
	bitmap[index] = bitmap[index] | (CLOSED >> offset * 2);
}

bool ContFramePool::check_empty(unsigned long frame) {
	
	char masks[4] = {0x3f, 0xcf, 0xf3, 0xfc};
	
	unsigned long index = (frame) / 4;
	unsigned long offset = (frame) % 4;
	
	return (bitmap[index] | masks[offset]) ==  masks[offset];
}

bool ContFramePool::check_head(unsigned long frame) {
	
	char masks[4] = {0xbf, 0xef, 0xfb, 0xfe};
	
	unsigned long index = (frame) / 4;
	unsigned long offset = (frame) % 4;
	
	return ((bitmap[index] | masks[offset]) ==  masks[offset]) && ((bitmap[index] & (HEAD >> offset * 2)) == (HEAD >> offset * 2));
}

bool ContFramePool::check_alloc(unsigned long frame) {
	
	char masks[4] = {0x7f, 0xdf, 0xf7, 0xfd};
	
	unsigned long index = (frame) / 4;
	unsigned long offset = (frame) % 4;
	
	return ((bitmap[index] | masks[offset]) ==  masks[offset]) && ((bitmap[index] & (ALLOCATED >> offset * 2)) == (ALLOCATED >> offset * 2));
}

ContFramePool* ContFramePool::FramePools[10];
int ContFramePool::PoolCount = 0;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    baseFrameNo = _base_frame_no;
    nFrames = _n_frames;
    nFreeFrames = _n_frames;
    infoFrameNo = _info_frame_no;
	nInfoFrames = _n_info_frames;
    
	// Bitmap must fit in a single frame!
    assert(nFrames <= FRAME_SIZE * 4);
	
    if(infoFrameNo == 0) {
        bitmap = (unsigned char *) (baseFrameNo * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (infoFrameNo * FRAME_SIZE);
    }    
    
	// Number of frames must be "fill" the bitmap!
    assert ((nFrames % 4 ) == 0);
	
    // Everything ok. Proceed to mark all bits in the bitmap
    for(int i=0; i < nFrames / 4; i++) {
        bitmap[i] = EMPTY;
    }
    
    // Mark the first frame as being used if it is being used
    if(infoFrameNo == 0) {
		assign_head(0);
		for (int i = 1; i < nInfoFrames; i++) {
			assign_alloc(i);
			nFreeFrames--;
		}
    }
	else if (infoFrameNo >= baseFrameNo && infoFrameNo < baseFrameNo + nFrames) {
		unsigned long cap;
		if (infoFrameNo + nInfoFrames - 1 < baseFrameNo + nFrames) {
			cap = infoFrameNo + nInfoFrames;
		}
		else {
			cap = baseFrameNo + nFrames;
		}
		
		assign_head(infoFrameNo - baseFrameNo);
		for (unsigned long i = infoFrameNo - baseFrameNo + 1; i < cap - baseFrameNo; i++) {
			assign_alloc(i);
			nFreeFrames--;
		}
	}
    
	if (PoolCount < 10) {
		FramePools[PoolCount] = this;
		PoolCount++;
	}
	
    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    assert(nFrames > _n_frames);
	
	unsigned int streak = 0;
	unsigned long start = 0;
	
	for (unsigned long i = 0; i < nFrames; i++) {
		
		if (check_empty(i)) {
			
			streak++;
		}
		else {
			streak = 0;
			start = i + 1;
		}
		
		if (streak == _n_frames) {
			break;
		}
	}
	
	if (streak != _n_frames) {
		return 0;
	}
	else {
		assign_head(start);
	
		for (int i = 1; i < _n_frames; i++) {
			assign_alloc(start + i);
		}
	}
	
	return start + baseFrameNo;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    for (unsigned long i = 0; i < _n_frames; i++) {
		assert(check_empty(_base_frame_no + i - baseFrameNo));
		assign_closed(_base_frame_no + i - baseFrameNo);
	}
}

void ContFramePool::release(unsigned long _first_frame_no) {
	assert(check_head(_first_frame_no - baseFrameNo));
		
	assign_empty(_first_frame_no - baseFrameNo);
	unsigned long curr = _first_frame_no - baseFrameNo + 1;
	
	while (check_alloc(curr)) {
		assign_empty(curr);
		curr++;
	}
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
	assert(PoolCount >= 0);
	
    for (int i = 0; i < PoolCount; i++) {
		if (_first_frame_no >= FramePools[i]->baseFrameNo && 
			_first_frame_no < FramePools[i]->baseFrameNo + FramePools[i]->nFrames) {
			
			FramePools[i]->release(_first_frame_no);
			
			break;
		}
	}
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    unsigned long frames = _n_frames / (FRAME_SIZE * 4);
	
	if (frames * FRAME_SIZE != _n_frames) {
		frames++;
	}
	
	return frames;
}
