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
ContFramePool* ContFramePool::head;
ContFramePool* ContFramePool::tail;
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no) {
   unsigned int bitmap_index = (_frame_no - base_frame_no) / 4;
   unsigned char mask = 0b11 << ((_frame_no % 4) * 2);


    if ((bitmap[bitmap_index] & mask) == 0) {
        return FrameState::Free;
    } else if ((bitmap[bitmap_index] & mask) == mask) {
        return FrameState::HoS;
    } else {
        return FrameState::Used;
    }
   
    
}

void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) {
   unsigned int bitmap_index = (_frame_no - base_frame_no) / 4;
   unsigned char mask = 0b11 << (((_frame_no -base_frame_no) % 4) * 2);
     switch(_state) {
        case FrameState::Used:
            bitmap[bitmap_index] = (bitmap[bitmap_index] & ~mask) | ((0b01 << ((_frame_no % 4) -base_frame_no * 2))& mask);
            break;
        case FrameState::Free:
            bitmap[bitmap_index] = (bitmap[bitmap_index] & ~mask) | ((0b00 << ((_frame_no -base_frame_no % 4) * 2))& mask);
            break;
        case FrameState::HoS:
            bitmap[bitmap_index] = (bitmap[bitmap_index] & ~mask) | ((0b10 << ((_frame_no -base_frame_no % 4) * 2))& mask);
            break;
    }
    
}

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    info_frame_no = _info_frame_no;

    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    //set in
    // unsigned long bitmap_size = (_n_frames + 7) / 8; // Round up division

   

    // Everything ok. Proceed to mark all frame as free.
    for(int fno = 0; fno < _n_frames; fno++) {
        set_state(fno, FrameState::Free);
    }
    
    // Mark the first frame as being used if it is being used
    if(_info_frame_no == 0) {
        set_state(0, FrameState::Used);
        nFreeFrames--;
    }
     // Iterate over the linked list to add frames
    if(head=nullptr){
        head=this;
        next=nullptr;
    }else{
        ContFramePool* curr_ptr = head;
        while(curr_ptr->next!=nullptr){
            curr_ptr = next;
        }
        curr_ptr->next = this;
        tail = this;
        this->next = nullptr;
    }
    // Console::puts("ContframePool::Constructor not implemented!\n");
    // assert(false);
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!

    unsigned long count = 0; // Counter for the number of consecutive free frames.
    unsigned long start_frame = 0; // Starting frame number of the consecutive free frames.

    for (unsigned long frame_no = 0; frame_no < nframes; ++frame_no) {
        if (get_state(frame_no) == FrameState::Free) {
            if (count == 0) {
                start_frame = frame_no;
            }
            count++;
            if (count == _n_frames) {
                // Mark the first one as HEAD-OF-SEQUENCE and the rest as ALLOCATED.
                set_state(start_frame, FrameState::HoS);
                for (unsigned long i = 1; i < _n_frames; ++i) {
                    set_state(start_frame + i, FrameState::Used);
                }
                return start_frame + base_frame_no;
            }
        } else {
            count = 0; // Reset the counter if we encounter a used frame.
        }
    }

    // If we reach here, it means we couldn't find a sequence of _n_frames free frames.
    return 0;
    //Console::puts("ContframePool::get_frames not implemented!\n");
    //assert(false);
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    Console::puts("ContframePool::mark_inaccessible not implemented!\n");
    assert(false);
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    Console::puts("ContframePool::release_frames not implemented!\n");
    assert(false);
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    return (_n_frames / 32768 + (_n_frames % 32768 > 0 ? 1 : 0));
    Console::puts("ContframePool::need_info_frames not implemented!\n");
    assert(false);
}
