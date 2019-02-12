type bot;

[@bs.deriving abstract]
type socketOption = {
  host: string,
  port: int
};

type event;
[@bs.send] external stopPropagation: event => unit = "";

type socket;

module MessageRequest {
  [@bs.deriving abstract]
  type content = {
    message_type: string,
    user_id: string,
    raw_message: string
  };
};

module GroupInviteRequest {
  [@bs.deriving abstract]
  type content = {
    user_id: string,
    group_id: string,
    flag: string
  };
};

type messageHandlerReturn = option(string);
type websocketType = string;

[@bs.new] [@bs.module "cq-websocket"] external createBot : socketOption => bot = "CQWebSocket";

[@bs.send]
external on : (
    bot,
    [@bs.string]
    [
      | [@bs.as "socket.error"] `socketError((websocketType, Js.Exn.t) => unit)
      | [@bs.as "socket.connecting"] `socketConnecting(websocketType => unit)
      | [@bs.as "socket.connect"] `socketConnect((websocketType, socket, int) => unit)
      | [@bs.as "socket.failed"] `socketFailed((websocketType, int) => unit)
      | [@bs.as "socket.close"] `socketClose((websocketType, int, string) => unit)

      | `ready(unit => unit)
      | `error(Js.Exn.t => unit)

      | [@bs.as "message.group"] `messageGroup((event, MessageRequest.content) => messageHandlerReturn)
      | [@bs.as "message.discuss"] `messageDiscuss((event, MessageRequest.content) => messageHandlerReturn)
      | [@bs.as "message.group.@.me"] `messageGroupAtMe((event, MessageRequest.content) => messageHandlerReturn)
      | [@bs.as "message.discuss.@.me"] `messageDiscussAtMe((event, MessageRequest.content) => messageHandlerReturn)
      | [@bs.as "message.private"] `messagePrivate((event, MessageRequest.content) => messageHandlerReturn)

      | [@bs.as "request.group.invite"] `requestGroupInvite(GroupInviteRequest.content => unit)
    ]
  ) => bot = "";

module PrivateMsg {
  [@bs.deriving abstract]
  type content = {
    user_id: string,
    message: string,
    auto_escape: bool
  };
};

module GroupInviteResponse {
  [@bs.deriving abstract]
  type content = {
    flag: string,
    sub_type: string,
    approve: bool
  };
};

[@bs.send]
external call : (
    bot,
    [@bs.string]
    [
      | [@bs.as "send_private_msg"] `sendPrivateMsg(PrivateMsg.content)
      | [@bs.as "set_group_add_request"] `setGroupAddRequest(GroupInviteResponse.content)
    ]
  ) => unit = "__call__";

[@bs.send] external connect : (bot) => unit = "";
