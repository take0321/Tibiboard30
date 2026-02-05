# この一行で vibe_handler.c をビルド対象に加えます
target_sources(app PRIVATE src/vibe_handler.c)

# ZMKのヘッダーパスを通します
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/include)
