# source files
SET(SRCS
NetQueue.cpp
RtpQueue.cpp
ScreamRx.cpp
ScreamTx.cpp
VideoEnc.cpp
)

SET(HEADERS
NetQueue.h
RtpQueue.h
ScreamRx.h
ScreamTx.h
VideoEnc.h
)


INCLUDE_DIRECTORIES(
${screamIncludes}
)

LINK_DIRECTORIES(
${screamLink}
)

ADD_LIBRARY( scream STATIC ${SRCS} ${HEADERS})

