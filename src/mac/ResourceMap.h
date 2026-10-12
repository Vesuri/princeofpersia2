#ifndef POP2_RESOURCE_MAP_H
#define POP2_RESOURCE_MAP_H
// A borrowed resource-map view. No payload pointer, allocation or file I/O.
// The caller retains the immutable map bytes for the view's lifetime.
class ResourceMap {
public:
    static const uint16_t maximumResources=768;
    struct Layout { uint32_t dataOffset,mapOffset,dataLength,mapLength; };
    struct Entry {
        uint32_t type,lengthOffset;
        int16_t id;
        uint8_t attrs,nameLength;
        const uint8_t* name;
    };
    static bool layout(const uint8_t* header,uint32_t sourceSize,Layout& out);
    bool open(const uint8_t* header,uint32_t sourceSize,const uint8_t* map,uint32_t mapSize);
    void close();
    uint16_t count() const { return count_; }
    // Index order is the original type/reference-list order, not a sorted copy.
    bool entry(uint16_t index,Entry& out) const;
    // Validate a length word read separately at Entry::lengthOffset. The body
    // can then be streamed at the returned offset, including a zero-byte body.
    bool payload(uint16_t index,uint32_t length,uint32_t& offset) const;
private:
    const uint8_t* map_=0;
    Layout layout_={};
    uint16_t count_=0;
};
#endif
