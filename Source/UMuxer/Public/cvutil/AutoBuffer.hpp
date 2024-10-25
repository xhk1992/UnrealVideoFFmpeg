#ifndef __AUTO_BUFFER_HPP__
#define __AUTO_BUFFER_HPP__
#include <cstddef>
#include <iostream>

template <typename _Tp,size_t fixed_size = 4096 / sizeof(_Tp) +8 > class AutoBuffer{
public:
    typedef _Tp value_type;
    enum{ buffer_padding =(int)((16+sizeof(_Tp) - 1) /sizeof(_Tp)) };

    //! the default contructor
    AutoBuffer(){
        ptr_ = buf_;
        size_= fixed_size;
        reset();
    }

    //! constructor taking the real buffer size
    AutoBuffer(size_t _size){
        ptr_ = buf_;
        size_ = fixed_size;
        allocate(_size);
        reset();
    }

    //! destructor. calls deallocate()
    ~AutoBuffer(){
        deallocate();
    }

    void allocate(size_t _size){
        if(_size <= size_)
            return;
        deallocate();
        if(_size > fixed_size){
            ptr_= new _Tp[_size];
            size_ = _size;
        }
        reset();
    }

    void deallocate() {
        if (ptr_ != buf_) {
            delete[]ptr_;
            ptr_ = buf_;
            size_ = fixed_size;
        }
    }

    //!returns pointer to the real buffer.stack-allocated or head-allocated
    operator _Tp* (){
        return ptr_;
    }

    //! return read-only pointer to the real buffer ,stacl-allocated or head-allocated
    operator const _Tp*() const{
        return ptr_;
    }

    //! returns buffer size 
    size_t size(){
        return size_;
    }

    //! set buffer to null
    void reset(){
        if(ptr_ && size_)
            memset(ptr_,0,size_ * sizeof(_Tp));
    }

    //! get pointer address
    _Tp** get_addr(){
        return &ptr_;
    }
protected:
    //! pointer to the real buffer,can point to buf_ if the buffer is samll enough

    _Tp* ptr_;
    //! size of the real buffer
    size_t size_;
    //! pre-allocated buffer
    _Tp buf_[fixed_size + buffer_padding];

} ;
#endif // !_AUTO_BUFFER_HPP__
