type logger;

[@bs.deriving abstract]
type streamOption = {
  level: string,
  [@bs.as "type"] type_: string,
  stream: Stream.stream
};

[@bs.deriving abstract]
type loggerOption = {
  name: string,
  streams: array(streamOption)
};

[@bs.module "bunyan"] external createLogger : loggerOption => logger = "createLogger";
