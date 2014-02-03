#include "RemoteServiceListener.h"
#include "ServiceDiscovery.h"
#include "TCPManager.h"

void RemoteServiceListener::handleMessage() {
	ByteArray array;
	array.resize(65536);

	int bytes = read( m_broadcastFileDescriptor, array.getData(), array.size() );
	assert( bytes <= static_cast<int>( array.size() ) );

	array.resize(bytes);
	m_tcpManager.getServiceDiscoveryMessageDecoder().decodeMessage( array.getData(), array.size() );
}
