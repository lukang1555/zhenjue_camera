uint32 device_id

uint32 height
uint32 width

uint32 data_size            # 实际有效字节数（<=MAX_DATA_SIZE）
uint8[11059200] data

uint64 pts_ns
uint64 receive_ns

# 常量：便于 C++ 端检查/限制，避免硬编码
uint32 MAX_WIDTH=2560
uint32 MAX_HEIGHT=1440
uint32 MAX_DATA_SIZE=11059200