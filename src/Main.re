open CQWebSocket;
open Log;
let config = Config.config;

let qqoption = socketOption(
  ~host=config.host,
  ~port=config.port
);
let qqbot = createBot(qqoption);

let formatDice = (numDices, numFaces, dices) => {
  {j|$(numDices)d$(numFaces): |j}
  ++
  if (numDices == 1) {
    string_of_int(List.nth(dices, 0))
  } else {
    String.concat(" + ", dices |> List.map(n => string_of_int(n)))
    ++
    " = "
    ++
    string_of_int(List.fold_left((x, y) => x + y, 0, dices))
  }
}

type diceMessageGroup('a) = {
  template: Js.Re.t,
  numDicesGet: 'a => string,
  numFacesGet: 'a => string,
  numTimesGet: 'a => option(string)
};

type diceMessage('a) = {
  captureGroup: 'a,
  message: string
};

let parseMessage = (group, message) => {
  let matches = message |> Js.String.match(group.template);
  switch(matches) {
    | Some(capture) => switch(RegExp.toGroupCapture(capture)->RegExp.groupsGet) {
      | Some(g) => {
        let numDices = try(g->(group.numDicesGet)->int_of_string) { | _ => 1 };
        let numFaces = try(g->(group.numFacesGet)->int_of_string) { | _ => 100 };
        if(numDices < 1 || numDices > 100) {
          None
        } else {
          let numTimes = switch(g->(group.numTimesGet)) {
            | Some(numTimesStr) => numTimesStr->int_of_string
            | None => 1
          };
          Array.init(numTimes, _ => 
            Random.cryptoDice(numDices, numFaces)
            |> formatDice(numDices, numFaces))
          |> Array.to_list
          |> String.concat("\n")
          |> m => Some({captureGroup: g, message: m})
        }
      }
      | None => None
    }
    | None => None
  }
};

let handleGroup = (_, context) => {
  open MessageRequest;
  module GroupDiceRequestGroup {
    let template = [%bs.re "/^\s*(?<isSecret>s?)((?<numTimes>[1-9]\d*)\*)?(?<numDices>([1-9]\d*)?)d(?<numFaces>([1-9]|[1-9][0-9]|100)?)\s*$/"];
    [@bs.deriving abstract]
    type group = {
      isSecret: string,
      [@bs.optional] numTimes: string,
      numDices: string,
      numFaces: string
    };
  };
  let messageType = context->message_typeGet;
  let message = context->raw_messageGet;
  let sender = context->user_idGet;
  logDebug({j|Received $messageType dice request $message from $sender|j}, None);

  message
  |> parseMessage({
    template: GroupDiceRequestGroup.template,
    numDicesGet: GroupDiceRequestGroup.numDicesGet,
    numFacesGet: GroupDiceRequestGroup.numFacesGet,
    numTimesGet: GroupDiceRequestGroup.numTimesGet
  })
  |> m => switch(m) {
    | Some({captureGroup: groups, message: diceResult}) => switch(groups->GroupDiceRequestGroup.isSecretGet) {
        | "s" => {
          let privateMsg = PrivateMsg.content(
            ~user_id=sender,
            ~message=diceResult,
            ~auto_escape=true
          );
          qqbot->call(`sendPrivateMsg(privateMsg));
          message
          |> String.trim
          |> m => {j|$m: ???|j}
        }
        | _ => diceResult
      }
      |> m => Some({j|[CQ:at,qq=$sender]\n$m|j})
    | None => None
  }
};

let handleGroupAtMe = (e, context) => {
  open MessageRequest;
  stopPropagation(e);
  let stripCQ = [%bs.re "/\[CQ:at[^\]]+\]/g"];
  context->raw_messageGet
  |> Js.String.replaceByRe(stripCQ, "")
  |> String.trim
  |> m => switch(Js.String.length(m)) {
    | 0 => Some("[CQ:emoji,id=128587]")
    | x when x > 0 && x < 8 => {
        let rd = Random.cryptoDice(1, 3);
        switch(rd) {
          | [1] => Some(m ++ {js|可还行|js})
          | [2] => Some("那怎么办呀")
          | [3] => Some("QAQ")
          | _ => None
        }
    }
    | _ => Some({js|喵呜，人家看不懂啦|js})
  }
};

let handlePrivate = (_, context) => {
  open MessageRequest;
  module PrivateDiceRequestGroup {
    let template = [%bs.re "/^\s*((?<numTimes>[1-9]\d*)\*)?(?<numDices>([1-9]\d*)?)d(?<numFaces>([1-9]|[1-9][0-9]|100)?)\s*$/"];
    [@bs.deriving abstract]
    type group = {
      [@bs.optional] numTimes: string,
      numDices: string,
      numFaces: string
    };
  };
  let message = context->raw_messageGet;
  let sender = context->user_idGet;
  logDebug({j|Received private dice request $message from $sender|j}, None);

  message
  |> parseMessage({
    template: PrivateDiceRequestGroup.template,
    numDicesGet: PrivateDiceRequestGroup.numDicesGet,
    numFacesGet: PrivateDiceRequestGroup.numFacesGet,
    numTimesGet: PrivateDiceRequestGroup.numTimesGet
  })
  |> m => switch(m) {
    | Some({captureGroup: _, message: diceResult}) => Some(diceResult)
    | None => None
  }
};

let handleInvite = (context) => {
  open GroupInviteRequest;
  let groupId = context->group_idGet;
  let inviterId = context->user_idGet;
  logInfo({j|Invited to group $groupId by $inviterId|j}, None);
  let requestApproval = GroupInviteResponse.content(
    ~flag=context->flagGet,
    ~sub_type="invite",
    ~approve=true
  );
  qqbot->call(`setGroupAddRequest(requestApproval))
};

qqbot
->on(`socketError((wsType, e) => {
  logError({j|[$wsType] Socket error|j}, Some(e))
}))
->on(`socketConnecting(wsType => {
  logInfo({j|[$wsType] Connecting|j}, None)
}))
->on(`socketConnect((wsType, _, attempts) => {
  logInfo({j|[$wsType] Connected after $attempts attempts|j}, None)
}))
->on(`socketFailed((wsType, attempts) => {
  switch(attempts > 10) {
    | true => Node.Process.exit(255)
    | false => logError({j|[$wsType] Failed to connect after $attempts attempts|j}, None)
  }
}))
->on(`socketClose((wsType, code, desc) => {
  logWarn({j|[$wsType] Connection closed: $code - $desc|j}, None)
}))

->on(`ready(() => {
  logInfo("Dice Goddess: Fate is on my hand.", None)
}))
->on(`error((e) => {
  logError("Dice Goddess has fallen.", Some(e))
}))

->on(`messageGroup(handleGroup))
->on(`messageDiscuss(handleGroup))
->on(`messageGroupAtMe(handleGroupAtMe))
->on(`messageDiscussAtMe(handleGroupAtMe))

->on(`messagePrivate(handlePrivate))

->on(`requestGroupInvite(handleInvite));

connect(qqbot);
