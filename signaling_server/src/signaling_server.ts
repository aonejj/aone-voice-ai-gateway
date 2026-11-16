import { EventEmitter } from 'events';
import { WebSocket } from 'ws';
import { randomUUID } from "node:crypto";
import { webcrypto } from "node:crypto";

import {
    S_WS_RET, 
    S_WS_STATE, 
    SignalingWebsocketServer     
} from './websocket_server.js';

import type {
    ISignalingWebsocketServerListener
} from './websocket_server.js';

import { Peer } from './peer.js'
import type { IPeerListener } from './peer.js'
import { RTCNode } from './rtcnode.js'
import type { IRTCNodeListener } from './rtcnode.js'

import { AgentNode } from './agent_node.js';
import type { IAgentNodeListener } from './agent_node.js';

import type {
    CommandMessage, 
    ResponseMessage, 
    SDPMessage, 
    CandidateMessage, 
    IceServerInfo,
    IceStateMessage,
    AgentEndpointInfo,
    AgentAddMessage,
    AgentRemoveMessage
} from './types/signaling_msg_type.js'

import {
    kAddPeerMsg,
    kFail, 
    kSuccess, 
    kRegistMsg, 
    kRegistMsgRes,
    kRemovePeerMsg,
    kOfferMsg,
    kCandidateMsg,
    kAnswerMsg,    
    kGoogleStunUrl,
    kAgentAddMsg,
    kAgentRemoveMsg
} from './types/signaling_msg_type.js';         

const kCreatePeer:string = "kCreatePeer";
const kCreateNode:string = "kCreateNode";
const kCreateAgent: string = "kCreateAgent";

export class SignalingServer extends EventEmitter implements ISignalingWebsocketServerListener, 
                                IPeerListener, IRTCNodeListener {

    private readonly _peers = new Map<string, Peer>;
    private readonly _nodes = new Map<string, RTCNode>;
    private readonly _agents = new Map<string, AgentNode>();
    private readonly _node_to_peers = new Map<RTCNode, Peer[]>;
    private _nodeIndex: number = 0;
    private _agentIndex: number = 0;

    private _ws_server:SignalingWebsocketServer|null = null;

    constructor() {
        super();

        this.on(kCreatePeer, this.onCreatePeer.bind(this));
        this.on(kCreateNode, this.onCreateNode.bind(this));
        this.on(kCreateAgent, this.onCreateAgent.bind(this));
    }

    startServer(port:number):void {
        if(this._ws_server !== null) {
            console.log(`SignalingServer websocket server already running`);
            return;
        }
        console.log(`SignalingServer startServer`);
        this._ws_server = new SignalingWebsocketServer(this);
        this._ws_server.start(port);
    }

    stopServer(): void {
        if(this._ws_server === null) {
            console.log(`SignalingServer websocket server already stop`);
            return;
        }
        console.log(`SignalingServer stopServer`);
        this._ws_server.stop();
    }

    onWebsocketServerListening(port: number): void {
        console.log('SignalingServer onListening port: ', port);
    }

    onConnectPeer(ws: WebSocket): void {
        console.log(`onConnectPeer`);
        const uuid:string =randomUUID();
        let peer:Peer = new Peer(ws, uuid, this);
        this._peers.set(uuid, peer);
        this.emit(kCreatePeer, peer);    
    }

    onConnectNode(ws: WebSocket): void {
        console.log(`onConnectNode`);
        const uuid:string = randomUUID();
        let node:RTCNode = new RTCNode(ws, uuid, this);
        this._nodes.set(uuid, node);
        this.emit(kCreateNode, node);
    }

    onConnectAgent(ws: WebSocket): void {
        console.log(`onConnectAgent`);

        const uuid: string = randomUUID();
        const agent: AgentNode = new AgentNode(ws, uuid, this);

        this._agents.set(uuid, agent);
        this.emit(kCreateAgent, agent);
    }

    onWebsocketServerClose(): void {
        console.log(`SignalingServer onWebsocketServerClose`);
        this._ws_server = null;
    }
    onWebSocketServerError(err: Error): void {
        console.log(`SignalingServer onWebSocketServerError`);
    }

    onPeerClose(peer:Peer): void {
        this._peers.delete(peer.getId());
        let node_id:string|null = peer.getRTCNodeId();
        if(node_id !== null) {
            let remove_peer_cmd:CommandMessage = {
                cmd:kRemovePeerMsg,
                id:peer.getId()
            };

            const node:RTCNode|null|undefined =  this._nodes.get(node_id);
            if(node !== null && node != undefined) {
                this._nodes.get(node_id)!.send(remove_peer_cmd);
                this.node_to_peer_remove(node, peer);
            }
        }
    }

    onPeerRegist(peer:Peer, jobj:any): void {
        console.log(`onPeerRegist`);

        const nodeEntry = this.getNextRTCNode();

        if (!nodeEntry) {
            console.error(`onPeerRegist fail: no available RTCNode`);

            const regist_res_msg: ResponseMessage = {
                cmd: kRegistMsgRes,
                result: kFail,
            };

            peer.send(regist_res_msg);
            return;
        }

        const agentEntry = this.getNextAgent();

        if (!agentEntry) {
            console.error(`onPeerRegist fail: no available Agent`);

            const regist_res_msg: ResponseMessage = {
                cmd: kRegistMsgRes,
                result: kFail,
            };

            peer.send(regist_res_msg);
            return;
        }

        const [node_id, node] = nodeEntry;
        const [agent_id, agent] = agentEntry;

        peer.setRTCNodeId(node_id);

        const add_peer_cmd: CommandMessage = {
            cmd: kAddPeerMsg,
            id: peer.getId(),
            agentId: agent_id,
        };

        node.send(add_peer_cmd);
        this.node_to_peer_add(node, peer);

        const stun_server:IceServerInfo = {
            url:kGoogleStunUrl
        };

        let regist_res_msg:ResponseMessage ={
            cmd: kRegistMsgRes,
            result:kSuccess,
            iceinfo:[stun_server]
        };
        peer.send(regist_res_msg);
    }

    onPeerOffer(peer:Peer, jobj:any): void {
        const node:RTCNode = this._nodes.get(peer.getRTCNodeId()!)!;
        const offer_msg:SDPMessage = {
            cmd: kOfferMsg,
            id: peer.getId(),
            sdp: jobj.sdp 
        };

        node.send(offer_msg);
    }   

    onPeerCandidate(peer:Peer, jobj:any):void {
        const node:RTCNode = this._nodes.get(peer.getRTCNodeId()!)!;
        const candidate_msg:CandidateMessage = {
            cmd: kCandidateMsg,
            id: peer.getId(),
            mline_index: jobj.mline_index,
            mid: jobj.mid,
            candidate: jobj.candidate
        };

        node.send(candidate_msg);
    }

    onRTCNodeRegist(node:RTCNode, jobj:any): void {
        console.log(`onRTCNodeRegist`);
        const stun_server:IceServerInfo = {
            url:kGoogleStunUrl
        };
        const node_id = node.getId();

        let regist_res_msg:ResponseMessage ={
            cmd: kRegistMsgRes,
            result:kSuccess,
            iceinfo:[stun_server],
            id:node_id,
            agents: this.get_agent_list()            
        };
        node.send(regist_res_msg);
    }

     onRTCNodeAddPeerRes(node:RTCNode, jobj:any):void {
        const peer_id = jobj.id;
        const result = jobj.result;
        if(result === "fail") {
            const peer = this._peers.get(peer_id);
            peer?.close();
            this.node_to_peer_remove(node, peer);
        }
     }

    onRTCNodeRemovePeerRes(node:RTCNode, jobj:any):void {

    }

    onRTCNodeAnswer(node:RTCNode, jobj:any):void {
        const peer_id = jobj.id;
        const peer = this._peers.get(peer_id);
        if(peer !== null && peer !== undefined) {
            let answer_msg:SDPMessage = {
                cmd:kAnswerMsg,
                sdp: jobj.sdp 
            }
            peer.send(answer_msg);
        }
    }

    onRTCNodeCandidate(node:RTCNode, jobj:any):void {
        const peer_id = jobj.id!;
        const peer = this._peers.get(peer_id);
        if(peer !== null && peer !== undefined) {
            let candidate_msg:CandidateMessage = {
                cmd:kCandidateMsg,
                mline_index: jobj.mline_index,
                mid: jobj.mid,
                candidate: jobj.candidate
            }
            peer.send(candidate_msg);
        }
    }

    onRTCNodeIceState(node:RTCNode, jobj:any):void {
        console.log(`[RTCNode ${node.getId()}] IceState ${jobj.state} desc ${jobj.desc}.`);
    }

    onRTCNodeClose(node:RTCNode): void {
        this._nodes.delete(node.getId());

        const peers =  this._node_to_peers.get(node);
        if(peers) {
            for(const peer of peers) {
                peer.close();
            }
        }

        this._node_to_peers.delete(node);
    }

    // ----------------------------------------------------------------------
    // AgentNode
    // ----------------------------------------------------------------------

    onAgentNodeRegist(agent: AgentNode, jobj: any): void {
        console.log(`[Agent ${agent.getId()}] onAgentNodeRegist`);

        const endpoint = jobj.endpoint;

        if (typeof endpoint !== "string" || endpoint.trim().length === 0) {
            console.log(`[Agent ${agent.getId()}] Invalid endpoint`);

            const regist_res_msg: ResponseMessage = {
                cmd: kRegistMsgRes,
                result: kFail
            };

            agent.send(regist_res_msg);
            agent.close();
            return;
        }

        const normalized_endpoint = endpoint.trim();

        agent.setEndpoint(normalized_endpoint);

        const regist_res_msg: ResponseMessage = {
            cmd: kRegistMsgRes,
            result: kSuccess,
            id: agent.getId()
        };

        agent.send(regist_res_msg);

        console.log(
            `[Agent ${agent.getId()}] Registered endpoint: ${normalized_endpoint}`
        );

        // Notify already connected RTCNodes about the new Agent.
        const agent_add_msg: AgentAddMessage = {
            cmd: kAgentAddMsg,
            id: agent.getId(),
            endpoint: normalized_endpoint
        };

        this.broadcast_to_RTCNodes(agent_add_msg);
    }

    onAgentNodeClose(agent: AgentNode): void {
        console.log(`[Agent ${agent.getId()}] onAgentNodeClose`);

        const was_registered = agent.isRegistered();

        this._agents.delete(agent.getId());

        if (!was_registered) {
            return;
        }

        // Notify RTCNodes to remove the disconnected Agent.
        const agent_remove_msg: AgentRemoveMessage = {
            cmd: kAgentRemoveMsg,
            id: agent.getId()
        };

        this.broadcast_to_RTCNodes(agent_remove_msg);
    }


    async onCreatePeer(peer:Peer): Promise<void> {
        await Promise.resolve();
        console.log(`onCreatePeer`);
        if(peer.isOpen() === false) {
            console.log(`onCreatePeer peer !isOpen`);
            this._peers.delete(peer.getId());
        }
    }

    async onCreateNode(node:RTCNode): Promise<void> {
        await Promise.resolve();

        console.log(`onCreateNode`); 

        if(node.isOpen() === false) {
            this._nodes.delete(node.getId());
        }
    }

    async onCreateAgent(agent: AgentNode): Promise<void> {
        await Promise.resolve();

        console.log(`onCreateAgent`);

        if (agent.isOpen() === false) {
            console.log(`onCreateAgent agent !isOpen`);
            this._agents.delete(agent.getId());
        }
    }    

    private node_to_peer_add(node:RTCNode, peer:Peer):void {
        if(!this._node_to_peers.has(node)) {
            this._node_to_peers.set(node, []);
        }
        this._node_to_peers.get(node)?.push(peer);
    }    

    private node_to_peer_remove(node:RTCNode, peer?:Peer):void {
        if (!peer) {
            return;
        }

        if(!this._node_to_peers.has(node)) {
            return;
        }
        const idx = this._node_to_peers.get(node)?.findIndex(item=>item === peer);
        if(idx !== -1 && idx != undefined) {
            this._node_to_peers.get(node)?.splice(idx, 1);
        }
    }

    private get_agent_list(): AgentEndpointInfo[] {
        const agents: AgentEndpointInfo[] = [];

        for (const [id, agent] of this._agents) {
            if (!agent.isOpen() || !agent.isRegistered()) {
                continue;
            }

            const endpoint = agent.getEndpoint();

            if (endpoint === null) {
                continue;
            }

            agents.push({
                id,
                endpoint
            });
        }

        return agents;
    }

    private broadcast_to_RTCNodes(message: AgentAddMessage | AgentRemoveMessage): void {
        for (const [id, node] of this._nodes) {
            if (!node.isOpen()) {
                continue;
            }

            console.log(
                `[RTCNode ${id}] Send Agent message: ${message.cmd}`
            );

            node.send(message);
        }
    }
    
    private getNextRTCNode(): [string, RTCNode] | undefined {
        const nodes = Array.from(this._nodes.entries());

        if (nodes.length === 0) {
            return undefined;
        }

        const entry = nodes[this._nodeIndex % nodes.length];

        this._nodeIndex = (this._nodeIndex + 1) % nodes.length;

        return entry;
    }    

    private getNextAgent(): [string, AgentNode] | undefined {
        const agents = Array.from(this._agents.entries());

        if (agents.length === 0) {
            return undefined;
        }

        const entry = agents[this._agentIndex % agents.length];

        this._agentIndex = (this._agentIndex + 1) % agents.length;

        return entry;
    }    
}