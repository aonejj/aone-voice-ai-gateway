
import { WebSocket, type RawData } from "ws";

import {
    kRegistMsg,
    kRegistMsgRes,
    kSuccess,
    kFail
} from "./types/signaling_msg_type.js";

export interface IAgentNodeListener {
    onAgentNodeClose(agent: AgentNode): void;
    onAgentNodeRegist(agent: AgentNode, jobj: any): void;
}

export class AgentNode {
    private readonly _ws: WebSocket;
    private readonly _id: string;
    private readonly _listener: IAgentNodeListener;

    private _endpoint: string | null = null;

    constructor(ws: WebSocket, id: string, listener: IAgentNodeListener) {
        this._ws = ws;
        this._id = id;
        this._listener = listener;

        this._ws.on('message', this.onMessage.bind(this));
        this._ws.on('error', this.onError.bind(this));
        this._ws.on('close', this.onClose.bind(this));
        this._ws.on('ping', this.onPing.bind(this));
        this._ws.on('pong', this.onPong.bind(this));
    }

    getId(): string {
        return this._id;
    }

    getEndpoint(): string | null {
        return this._endpoint;
    }

    setEndpoint(endpoint: string): void {
        this._endpoint = endpoint;
    }

    isRegistered(): boolean {
        return this._endpoint !== null;
    }

    isOpen(): boolean {
        return this._ws.readyState === WebSocket.OPEN;
    }

    send(jobj: any): void {
        if (!this.isOpen()) {
            return;
        }

        try {
            const msg = JSON.stringify(jobj);
            this._ws.send(msg);
        } catch (error) {
            console.log(
                `[AgentNode ${this._id}] Failed to stringify message for sending: `,
                jobj,
                error
            );
        }
    }

    close(): void {
        if (!this.isOpen()) {
            return;
        }

        this._ws.close();
    }

    onMessage(data: RawData, isBinary: boolean): void {
        if (isBinary) {
            console.warn(
                `[AgentNode ${this._id}] Received binary data, ignoring.`
            );
            return;
        }

        const msg = data.toString();
        let jobj: any;

        console.log(`[AgentNode ${this._id}] Received message: ${msg}`);

        try {
            jobj = JSON.parse(msg);
        } catch (error) {
            console.error(
                `[AgentNode ${this._id}] Failed to parse incoming message as JSON: ${msg}. Error: ${error}`
            );
            return;
        }

        console.log(
            `[AgentNode ${this._id}] Received ${jobj.cmd} command.`
        );

        switch (jobj.cmd) {
            case kRegistMsg:
                this._listener.onAgentNodeRegist(this, jobj);
                break;

            default:
                console.warn(
                    `[AgentNode ${this._id}] Unknown command: ${jobj.cmd}`
                );
                break;
        }
    }

    onError(error: Error): void {
        console.error(`[AgentNode ${this._id}] WebSocket error:`, error);
    }

    onClose(code: number, reason: Buffer): void {
        console.log(
            `[AgentNode ${this._id}] onClose code=${code}, reason=${reason.toString()}`
        );

        this._listener.onAgentNodeClose(this);
    }

    onPing(data: Buffer): void {

    }

    onPong(data: Buffer): void {

    }
}
