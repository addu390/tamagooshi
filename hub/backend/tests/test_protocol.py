from src.wire import protocol


def test_parse_envelope_roundtrips_a_valid_message():
    raw = protocol.envelope("time.set", protocol.TimeSet(epoch=1700000000, tz_offset=330))
    env = protocol.parse_envelope(raw)
    assert env is not None
    assert env.type == "time.set"
    body = protocol.TimeSet.model_validate(env.body)
    assert body.epoch == 1700000000
    assert body.tz_offset == 330


def test_parse_envelope_rejects_non_json():
    assert protocol.parse_envelope("not json") is None


def test_parse_envelope_rejects_missing_required_fields():
    assert protocol.parse_envelope('{"type":"page.ack"}') is None
