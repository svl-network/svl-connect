# GenerateChangelog.cmake
# Generates a Markdown changelog at compile/build time from git log or GitHub API

if(NOT DEFINED OUTPUT_FILE)
    set(OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/../launcher/resources/documents/changelog.md")
endif()

if(NOT DEFINED REPO_ROOT)
    set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
endif()

set(CHANGELOG_CONTENT "")
set(GENERATED_SUCCESS FALSE)

# 1. Try local git log if git and .git are available
find_program(GIT_EXECUTABLE NAMES git git.exe PATHS "C:/Program Files/Git/cmd" "C:/Program Files/Git/bin")
if(GIT_EXECUTABLE AND EXISTS "${REPO_ROOT}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" log -n 30 "--pretty=format:* **%s** `(%h)` - *%an*, %as"
        WORKING_DIRECTORY "${REPO_ROOT}"
        OUTPUT_VARIABLE GIT_LOG_OUT
        RESULT_VARIABLE GIT_LOG_RES
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_LOG_RES EQUAL 0 AND NOT "${GIT_LOG_OUT}" STREQUAL "")
        string(CONCAT CHANGELOG_CONTENT
            "# Sunveil Network - Changelog & Patch Notes\n\n"
            "### Live Commit History (main branch)\n\n"
            "${GIT_LOG_OUT}\n\n"
            "---\n*Compiled directly from repository source tree.*\n"
        )
        set(GENERATED_SUCCESS TRUE)
        message(STATUS "SVL Changelog generated from local git history.")
    endif()
endif()

# 2. If git failed, try GitHub REST API
if(NOT GENERATED_SUCCESS)
    set(GITHUB_API_URL "https://api.github.com/repos/SunveilNetwork/SVL-Connect/commits?per_page=25")
    set(TEMP_JSON "${CMAKE_CURRENT_BINARY_DIR}/github_commits.json")
    file(DOWNLOAD "${GITHUB_API_URL}" "${TEMP_JSON}"
        TIMEOUT 5
        STATUS DOWNLOAD_STATUS
        HTTPHEADER "User-Agent: SunveilConnect-Build/1.0"
    )
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(STATUS_CODE EQUAL 0 AND EXISTS "${TEMP_JSON}")
        file(READ "${TEMP_JSON}" JSON_DATA)
        string(JSON NUM_COMMITS ERROR_VARIABLE JSON_ERR LENGTH "${JSON_DATA}")
        if(NOT JSON_ERR AND NUM_COMMITS GREATER 0)
            set(CHANGELOG_CONTENT "# Sunveil Network - Changelog & Patch Notes\n\n### Latest GitHub Commits\n\n")
            math(EXPR MAX_IDX "${NUM_COMMITS} - 1")
            if(MAX_IDX GREATER 24)
                set(MAX_IDX 24)
            endif()
            foreach(IDX RANGE 0 ${MAX_IDX})
                string(JSON C_MSG GET "${JSON_DATA}" ${IDX} "commit" "message")
                string(JSON C_SHA GET "${JSON_DATA}" ${IDX} "sha")
                string(JSON C_AUTHOR GET "${JSON_DATA}" ${IDX} "commit" "author" "name")
                string(JSON C_DATE GET "${JSON_DATA}" ${IDX} "commit" "author" "date")
                string(SUBSTRING "${C_SHA}" 0 7 SHORT_SHA)
                # First line of commit message only
                string(REGEX REPLACE "(\r?\n).*" "" FIRST_LINE "${C_MSG}")
                string(SUBSTRING "${C_DATE}" 0 10 SHORT_DATE)
                string(APPEND CHANGELOG_CONTENT "* **${FIRST_LINE}** `(${SHORT_SHA})` - *${C_AUTHOR}*, ${SHORT_DATE}\n")
            endforeach()
            string(APPEND CHANGELOG_CONTENT "\n---\n*Fetched from GitHub REST API.*\n")
            set(GENERATED_SUCCESS TRUE)
            message(STATUS "SVL Changelog fetched from GitHub REST API.")
        endif()
        file(REMOVE "${TEMP_JSON}")
    endif()
endif()

# 3. Fallback changelog if both offline and no git
if(NOT GENERATED_SUCCESS)
    string(CONCAT CHANGELOG_CONTENT
        "# Sunveil Network - Changelog & Patch Notes\n\n"
        "### Recent Updates & Milestones\n\n"
        "* **feat(sync):** High-performance FiveM-style mod synchronization engine with SHA-256 validation `(1d9150d)` - *Sunveil Team*, 2026-08-18\n"
        "* **feat(ui):** Modernized dark-slate UI, real-time server status, and dynamic server branding `(2578328)` - *Sunveil Team*, 2026-08-18\n"
        "* **feat(news):** Compile-time changelog generator and embedded patch notes overlay `(13d4aa7)` - *Sunveil Team*, 2026-08-18\n"
        "* **feat(security):** Zero-trust mod quarantine and local instance isolation `(8302c24)` - *Sunveil Team*, 2026-08-18\n"
        "* **fix(network):** Master API ping latency, heartbeat failover, and timeout handling `(6aa2c3d)` - *Sunveil Team*, 2026-08-18\n\n"
        "---\n*Sunveil Connect Production Build.*\n"
    )
    message(STATUS "SVL Changelog generated using built-in release notes fallback.")
endif()

# Write output file
file(WRITE "${OUTPUT_FILE}" "${CHANGELOG_CONTENT}")
