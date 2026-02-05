# app (ZMK本体) にソースファイルを追加する
target_sources(app PRIVATE src/vibe_handler.c)

# ZMKの内部ヘッダーを参照可能にする
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/include)
