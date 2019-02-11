let logger = {
  let prettyStream = {
    let stream = PrettyStream.prettyStream();
    Stream.pipe(stream, [%raw "process.stdout"]);
    stream
  };

  let streamOption = Bunyan.streamOption(
    ~level="info",
    ~type_="raw",
    ~stream=prettyStream
  );

  let loggerOption = Bunyan.loggerOption(
    ~name="dice-goddess",
    ~streams=[|streamOption|]
  );
  Bunyan.createLogger(loggerOption)
};

[@bs.send] external logDebug: (Bunyan.logger, string, unit) => unit = "debug";
[@bs.send] external logDebugWithExtra: (Bunyan.logger, string, 'a, unit) => unit = "debug";
[@bs.send] external logInfo: (Bunyan.logger, string, unit) => unit = "info";
[@bs.send] external logInfoWithExtra: (Bunyan.logger, string, 'a, unit) => unit = "info";
[@bs.send] external logWarn: (Bunyan.logger, string, unit) => unit = "warn";
[@bs.send] external logWarnWithExtra: (Bunyan.logger, string, 'a, unit) => unit = "warn";
[@bs.send] external logError: (Bunyan.logger, string, unit) => unit = "error";
[@bs.send] external logErrorWithExtra: (Bunyan.logger, string, 'a, unit) => unit = "error";
[@bs.send] external logFatal: (Bunyan.logger, string, unit) => unit = "fatal";
[@bs.send] external logFatalWithExtra: (Bunyan.logger, string, 'a, unit) => unit = "fatal";

let autoOverload = (normal, withExtra) => (message, extra) => switch(extra) {
  | Some(value) => withExtra(message, value, ())
  | None => normal(message, ())
};

let logDebug = (message, extra) => autoOverload(logDebug(logger), logDebugWithExtra(logger))(message, extra);
let logInfo = (message, extra) => autoOverload(logInfo(logger), logInfoWithExtra(logger))(message, extra);
let logWarn = (message, extra) => autoOverload(logWarn(logger), logWarnWithExtra(logger))(message, extra);
let logError = (message, extra) => autoOverload(logError(logger), logErrorWithExtra(logger))(message, extra);
let logFatal = (message, extra) => autoOverload(logFatal(logger), logFatalWithExtra(logger))(message, extra);
